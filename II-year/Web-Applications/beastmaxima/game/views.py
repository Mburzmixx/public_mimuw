import json
import random
from django.utils import timezone

from django.http import HttpResponse, JsonResponse
from django.shortcuts import render, redirect, get_object_or_404
from django.contrib.auth.decorators import login_required

from .models import Puzzle, GameSession

def lobby(request):
  if request.method == 'POST':
    size = int(request.POST.get('size', 4))
    return redirect('play', size=size)
  
  available_sizes = Puzzle.objects.values_list('size', flat=True).distinct().order_by('size')

  return render(request, 'game/lobby.html', {'available_sizes': available_sizes})
        
@login_required
def play(request, size):
  puzzles = list(Puzzle.objects.filter(size=size))

  if not puzzles:
    return render(request, 'game/play.html', {
      'error': 'No puzzles available for this size.'
    })

  puzzle = random.choice(puzzles)

  session = GameSession.objects.create(
    user=request.user,
    puzzle=puzzle
  ) 

  full_puzzle_data = {
    'board': puzzle.puzzle_data,
    'size': size,
    'difficulty': puzzle.difficulty,
    'id': puzzle.id
  }


  context = {
    'initial_board': json.dumps(full_puzzle_data),
    'session_id': session.id,
    'size': size,
  }
  return render(request, 'game/play.html', context)

@login_required
def validate_puzzle(request): 
  if request.method == 'POST':
    data = json.loads(request.body)
    
    session_id = data.get('sessionId')
    session_id = get_object_or_404(GameSession, id=session_id, user=request.user)

    puzzle = data.get('puzzle', {})
    size = puzzle.get('size', 0)
    board = puzzle.get('board', {})
    game_board = board.get('gameBoard')
    top = board.get('topClues')
    left = board.get('leftClues')
    bottom = board.get('bottomClues')
    right = board.get('rightClues')

    if not all([game_board, top, left, bottom, right]):
      return JsonResponse({'error': 'Invalid board data'}, status=400)
    
    errors = {}
    is_full = True

    try:
      for c in range(size):
        for r in range(size):
          if game_board[r][c] is None:
            is_full = False
            break
    except Exception as e:
      return JsonResponse({'error': 'Invalid board structure' + str(e)}, status=400)
  
    if not is_full:
      errors['board'] = 'Board is not fully filled'

    for i in range(size):
      row = game_board[i]
      col = [game_board[r][i] for r in range(size)]
      if len(set(row)) != size:
        errors[f'row_{i}'] = f'Row {i} does not contain unique values'
      if len(set(col)) != size:
        errors[f'col_{i}'] = f'Column {i} does not contain unique values'

    def countPrefixMaxima(line):
      count = 0
      prefixMaximum = 0
      for value in line:
        if value is None:
          return -1
        if value > prefixMaximum:
          count += 1
          prefixMaximum = value
      return count
    
    for i in range(size):
      row = game_board[i]
      col = [game_board[r][i] for r in range(size)]

      if left[i] is not None and countPrefixMaxima(row) != left[i]:
        errors[f'left_{i}'] = f'Left clue for row {i} is incorrect'
      if right[i] is not None and countPrefixMaxima(row[::-1]) != right[i]:
        errors[f'right_{i}'] = f'Right clue for row {i} is incorrect'
      if top[i] is not None and countPrefixMaxima(col) != top[i]:
        errors[f'top_{i}'] = f'Top clue for column {i} is incorrect'
      if bottom[i] is not None and countPrefixMaxima(col[::-1]) != bottom[i]:
        errors[f'bottom_{i}'] = f'Bottom clue for column {i} is incorrect'
      
    if errors:
      return JsonResponse({'puzzle_complete': False, 'errors': errors})

    if not session_id.is_completed:
      session_id.is_completed = True
      session_id.end_time = timezone.now()
      session_id.time_taken = session_id.end_time - session_id.start_time
      session_id.save()

    return JsonResponse({'puzzle_complete': True,
                         'time_taken': str(session_id.time_taken).split('.')[0]})
  else:
    return JsonResponse({"error": "Invalid request method"}, status=400)

def get_updated_ranking(request):
  top_sessions = GameSession.objects.filter(is_completed=True).order_by('-puzzle__size', 'time_taken')[:5]

  leaderboard_data = [
    {
      'username': session.user.username,
      'puzzle_size': session.puzzle.size,
      'difficulty': session.puzzle.difficulty,
      'formatted_time': session.formatted_time_taken,      
    }
    for session in top_sessions
  ]

  return JsonResponse({'global_leaderboard': leaderboard_data})

def get_updated_size_ranking(request, size):
  board_size = size

  if not board_size:
    return HttpResponse({'error': 'Size parameter is required'}, status=400)
  
  
  size_ranking = GameSession.objects.filter(
    is_completed=True,
    puzzle__size=board_size
  ).order_by('time_taken')[:5]

  size_leaderboard_data = [
    {
      'username': session.user.username,
      'puzzle_size': session.puzzle.size,
      'difficulty': session.puzzle.difficulty,
      'formatted_time': session.formatted_time_taken,      
    }
    for session in size_ranking
  ]

  return JsonResponse({'size_ranking': size_leaderboard_data})

