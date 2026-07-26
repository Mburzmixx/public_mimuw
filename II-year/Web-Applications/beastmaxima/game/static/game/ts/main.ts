type Cell = number | null;
type Clue = number | null;
type ClueKey = "topClues" | "bottomClues" | "leftClues" | "rightClues";
type ValidationResult = 
  | { valid: true }
  | { valid: false; errors: Record<string, string>};
type GameEngineResponse =
  | { puzzle_complete: true, time_taken: string }
  | { puzzle_complete: false; errors: Record<string, string> };
type Result<T> =
  | { ok: true;  data: T }
  | { ok: false; error: string; status?: number };

interface Board {
  gameBoard: Cell[][];
  topClues: Clue[];
  bottomClues: Clue[];
  leftClues: Clue[];
  rightClues: Clue[];
}

interface Puzzle {
  board: Board;
  size: number;
  difficulty: string;
  id: string;
}

interface RankingSession {
  username: string;
  puzzle_size: number;
  difficulty: string;
  formatted_time: string; 
}



let currentPuzzle: Puzzle = {
  board: {
    gameBoard: [],
    topClues: [],
    bottomClues: [],
    leftClues: [],
    rightClues: []
  },
  size: 0,
  difficulty: "",
  id: ""
};

let currentSessionId: string| null = null;
let initialPuzzle: Puzzle | null = null;

let timerInterval: number | null = null;
let secondsElapsed: number = 0;

function validatePuzzle(puzzle: Puzzle | null): ValidationResult {
  console.log("Validating puzzle:", puzzle);
  return { valid: true };
}

function initGame() {
  const boardElement = document.getElementById("board");
  const checkBtn = document.getElementById("check-board-btn");
  const resetBtn = document.getElementById("reset-board-btn");
  const timerElement = document.getElementById("game-timer");

  if (!boardElement || !checkBtn || !resetBtn || !timerElement) {
    console.error("Could not find board or check button or reset button or timer element in the DOM. Failed to initialize the game.");
    return;
  }

  let rawInitialPuzzleData = boardElement.dataset.initial;
  initialPuzzle = rawInitialPuzzleData ? JSON.parse(rawInitialPuzzleData) : null;

  secondsElapsed = 0;
  if (timerInterval) {
    clearInterval(timerInterval);
  }

  timerInterval = window.setInterval(() => {
      secondsElapsed++;
      const minutes = Math.floor(secondsElapsed / 60).toString().padStart(2, '0');
      const seconds = (secondsElapsed % 60).toString().padStart(2, '0');
      timerElement.textContent = `${minutes}:${seconds}`;
    }, 1000);

  currentSessionId = boardElement.dataset.sessionId || null;
  console.log("Current session ID:", currentSessionId);

  validatePuzzle(initialPuzzle);

  if (checkBtn) checkBtn.addEventListener("click", checkBoard);
  if (resetBtn) resetBtn.addEventListener("click", resetBoard);
  
  buildBoard(boardElement);
}

function buildBoard(boardElement: HTMLElement | null = document.getElementById("board")) {
  if (!boardElement) {
    console.error("Could not find board element in the DOM. Failed to build the board.");
    return;
  }

  if (initialPuzzle) {
    console.log("Loading initial puzzle from data attribute:", initialPuzzle);
    currentPuzzle = JSON.parse(JSON.stringify(initialPuzzle));
  } else {
    console.error("No initial puzzle data found. Starting with an empty board.");
    return;
  }

  validatePuzzle(currentPuzzle);
  const size = currentPuzzle.size;

  boardElement.innerHTML = "";
  boardElement.style.display = "grid";
  boardElement.style.gridTemplateColumns = `repeat(${size + 2}, 1fr)`;
  boardElement.style.gap = "8px";

  console.log("Building board with size:", size);
  for (let row = 0; row < size + 2; row++) {
    for (let col = 0; col < size + 2; col++) {
      let isTopRow = (row === 0);
      let isBottomRow = (row === size + 1);
      let isLeftCol = (col === 0);
      let isRightCol = (col === size + 1);

      if ((isTopRow && isLeftCol) || (isTopRow && isRightCol)
         || (isBottomRow && isLeftCol) || (isBottomRow && isRightCol)) {
        const corner = document.createElement("div");
        corner.classList.add("corner-cell");
        boardElement.appendChild(corner);
        continue;
      }
      
      const cell = document.createElement("input");
      cell.type = "text";
      cell.inputMode = "numeric";
      cell.maxLength = size >= 10 ? 2 : 1; 
      cell.dataset.row = row.toString();
      cell.dataset.col = col.toString();
      
      if (isTopRow) {
        setupClueCell(cell, "topClues", col - 1);
      } else if (isBottomRow) {
        setupClueCell(cell, "bottomClues", col - 1);
      } else if (isLeftCol) {
        setupClueCell(cell, "leftClues", row - 1);
      } else if (isRightCol) {
        setupClueCell(cell, "rightClues", row - 1);
      } else {
        setupBoardCell(cell, row - 1, col - 1);
      }

      boardElement.appendChild(cell);
    }
  }
}

function setupClueCell(cell: HTMLInputElement, clueArrayName: ClueKey, index: number) {
  cell.readOnly = true;
  const clueValue: Clue = currentPuzzle.board[clueArrayName][index];

  if (clueValue !== null) {
    cell.value = clueValue.toString();
    cell.classList.add("clue-input", "clue-number");
  } else {
    cell.value = "";
    cell.classList.add("clue-input", "clue-null");
  }
}

function setupBoardCell(cell: HTMLInputElement, row: number, col: number) {
  cell.classList.add("cell", "cell-input");

  const cellValue: Cell = currentPuzzle.board.gameBoard[row][col];
  if (cellValue !== null) {
    cell.value = cellValue.toString();
    cell.classList.add("filled");

    cell.readOnly = true;
    cell.classList.add("fixed-board-cell");
  } else {
    cell.value = "";
    cell.addEventListener("focus", () => {cell.select();});
    cell.addEventListener("input", () => handleInput(row, col, cell));
    cell.addEventListener("keydown", (event) => handleKeyDown(event, row + 1, col + 1, cell));
  }
}

function handleInput(row: number, col: number, cell: HTMLInputElement) {
  const value = cell.value.trim();

  if (value === "") {
    currentPuzzle.board.gameBoard[row][col] = null;
    cell.classList.remove("filled");
    return;
  }

  const num = parseInt(value, 10);

  if (isNaN(num) || num < 1 || num > currentPuzzle.size) {
    cell.value = ""; 
    currentPuzzle.board.gameBoard[row][col] = null; 
    cell.classList.remove("filled");
  } else {
    currentPuzzle.board.gameBoard[row][col] = num;
    cell.classList.add("filled");
  }
}

function handleKeyDown(event: KeyboardEvent, row: number, col: number, cell: HTMLInputElement) {
  let targetRow = row;
  let targetCol = col;

  switch (event.key) {
    case "ArrowUp":    targetRow = row - 1; break;
    case "ArrowDown":  targetRow = row + 1; break;
    case "ArrowLeft":  targetCol = col - 1; break;
    case "ArrowRight": targetCol = col + 1; break;
    case "Enter":      checkBoard(); return;
    default: return; 
  }

  const boardElement = document.getElementById("board");
  if (!boardElement) return;

  const targetCell = boardElement.querySelector(`input[data-row="${targetRow}"][data-col="${targetCol}"]`) as HTMLInputElement | null;
  
  if (targetCell && !targetCell.readOnly) {
    event.preventDefault();
    targetCell.focus();
    targetCell.select();
  }
}


function getCsrfToken(): string {
  const match = document.cookie.match(/csrftoken=([^;]+)/);
  return match ? match[1] : "";
}

async function apiFetch<T>(method: string, path: string, body?: unknown): Promise<Result<T>> {
  try {
    const options: RequestInit = {
      method,
      headers: {
        "Content-Type": "application/json",
        "X-CSRFToken":  getCsrfToken(),
      },
    };
    if (body !== undefined) {
      options.body = JSON.stringify(body);
    }

    const res = await fetch(path, options);

    if (!res.ok) {
      let message = `HTTP ${res.status}`;
      try {
        const err = await res.json();
        message = err.error ?? message;
      } catch { /* ignore */ }
        return { ok: false, error: message, status: res.status };
    }

    if (res.status === 204) {
      return { ok: true, data: {} as T };
    }

    const data: T = await res.json();
    return { ok: true, data };
  } catch (err) {
    return { ok: false, error: String(err) };
  }
}


async function checkBoard() {
  console.log("Checking board with current state:", currentPuzzle.board);
  let payload = {
    sessionId: currentSessionId,
    puzzle: currentPuzzle
  };
  
  const result = await apiFetch<GameEngineResponse>("POST", '/game/api/validate/', payload);

  if (result.ok) {
    if (result.data.puzzle_complete) {
      stopTimer();

      fetchUpdatedRanking();

      const boardElement = document.getElementById("board");
      if (boardElement && boardElement.dataset.size) {
        const sizeValue = parseInt(boardElement.dataset.size, 10);
        fetchSizeRanking(sizeValue);
      }

      showGameMessage("Congratulations!", `You've completed the puzzle!\nTime taken: ${result.data.time_taken}`,
        () => {window.location.href = "/lobby/";}
      );
      // alert(`Congratulations! You've completed the puzzle!\n Time taken: ${result.data.time_taken}` );
    } else {
      const errorString = Object.values(result.data.errors || {}).join("\n");
      showGameMessage("Keep Trying!", `There are still some errors in the puzzle.\nKeep trying!`);
      // alert(`There are still some errors in the puzzle. Keep trying!`);
      //\n Errors:\n${errorString}`
    }
  } else {
    showGameMessage("Server Error", `Error validating the board: ${result.error}`);
    // alert(`Error validating the board: ${result.error}`);
  }
}

function resetBoard(event?: Event) {
  console.log("Resetting board to initial state.");
  if (!initialPuzzle) {
    console.error("No initial puzzle data available. Cannot reset the board.");
    return;
  }
  
  currentPuzzle = JSON.parse(JSON.stringify(initialPuzzle));
  const boardElement = document.getElementById("board");
  buildBoard(boardElement);

  if (event && event.target instanceof HTMLElement) {
    event.target.blur();
  }
}

function showGameMessage(title: string, message: string, onCloseCallback?: () => void) {
  const overlay = document.getElementById("game-message");
  const titleElem = document.getElementById("game-message-title");
  const bodyElem = document.getElementById("game-message-body");
  const closeBtn = document.getElementById("game-message-close-btn");

  if (!overlay || !titleElem || !bodyElem || !closeBtn) {
    console.error("Game message elements not found in the DOM.");
    alert(`${title}\n\n${message}`);
    return;
  }

  titleElem.textContent = title;
  bodyElem.textContent = message;
  overlay.classList.remove("hidden");

  closeBtn.onclick = () => {
    overlay.classList.add("hidden");
    if (onCloseCallback) {
      onCloseCallback();
    }
  };
}

function stopTimer() {
  if (timerInterval !== null) {
    clearInterval(timerInterval);
    timerInterval = null;
  }
}

async function fetchUpdatedRanking() {
  const result = await apiFetch<{ global_leaderboard: RankingSession[] }>("GET", '/game/api/ranking/');

  if (result.ok) {
    const leaderBoard: RankingSession[] = result.data.global_leaderboard;
    console.log("Fetched updated global leaderboard:", leaderBoard);

    const rankingContainer = document.getElementById("ranking-container");
    if (!rankingContainer) {
      console.error("Ranking container element not found in the DOM.");
      return;
    }

    if (leaderBoard.length === 0) {
      rankingContainer.innerHTML = "<li class='ranking-item'>No completed games yet. Be the first to finish a puzzle!</li>";
      return;
    }

    let htmlContent = "";

    leaderBoard.forEach((session, index) => {
      htmlContent += `
        <li class="ranking-item">
          <div class="ranking-rank">${index + 1}</div>
          <div class="ranking-details">
            <a href="/accounts/profile/${session.username}/" class="player-link">${session.username}</a>
            <span class="puzzle-specs">
              ${session.puzzle_size}x${session.puzzle_size} | ${session.difficulty}
            </span>
          </div>
          <div class="ranking-time">${session.formatted_time}</div>
        </li>`;
    });

    rankingContainer.innerHTML = htmlContent;
  } else {
    console.error("Error fetching updated ranking:", result.error);
  }
}


async function fetchSizeRanking(size: number) {
  const result = await apiFetch<{ size_ranking: RankingSession[] }>("GET", `/game/api/ranking/size/${size}/`);

  if (result.ok) {
    const sizeRanking = result.data.size_ranking;

    const rankingContainer = document.getElementById("ranking-size-container");

    if (!rankingContainer) {
      console.error("Size ranking elements not found in the DOM.");
      return;
    }

    if (sizeRanking.length === 0) {
      rankingContainer.innerHTML = "<li class='ranking-item'>No completed games for this size yet. Be the first to finish a puzzle of this size!</li>";
      return;
    }

    let htmlContent = "";

    sizeRanking.forEach((session, index) => {
      htmlContent += `
        <li class="ranking-item">
          <div class="ranking-rank">${index + 1}</div>
          <div class="ranking-details">
            <a href="/accounts/profile/${session.username}/" class="player-link">${session.username}</a>
            <span class="puzzle-specs">
              ${session.puzzle_size}x${session.puzzle_size} | ${session.difficulty}
            </span>
          </div>
          <div class="ranking-time">${session.formatted_time}</div>
        </li>`;
    });

    rankingContainer.innerHTML = htmlContent;
  } else {
    console.error("Error fetching size-specific ranking:", result.error);
  }
}


document.addEventListener("DOMContentLoaded", initGame);
document.addEventListener("DOMContentLoaded", () => {
  const boardElement = document.getElementById("board");

  if (boardElement) {
    const size = boardElement.dataset.size;

    if (size) {
      const sizeValue = parseInt(size, 10);
      const sizeSection = document.getElementById("ranking-size-sidebar");
      const sizeTitleElem1 = document.getElementById("ranking-header-size");

      if (sizeSection && sizeTitleElem1) {
        sizeTitleElem1.innerText = `Best Times for ${sizeValue}x${sizeValue}`;
        fetchSizeRanking(sizeValue);
        sizeSection.classList.remove("hidden");
      }

      setInterval(() => {
        fetchSizeRanking(sizeValue);
        fetchUpdatedRanking();
      }, 15000);
    }
  }
  else {
    setInterval(fetchUpdatedRanking, 15000);
  }
}
);
