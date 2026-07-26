# Module containing utility stuff.
from json import load, dump
from wordfreq import word_frequency, top_n_list
from pandas import DataFrame, set_option
import time
import matplotlib.pyplot as plt
from pathlib import Path

OK = 0
BULBAPEDIA_URL = "https://bulbapedia.bulbagarden.net/wiki/"
repo_root = Path(__file__).resolve().parent.parent
dict_path = repo_root / "wiki_scraper" / "word-counts.json"


def update_word_counts(to_add: dict[str, int]):
    existing_counts = {}

    if dict_path.exists():
        with open(dict_path, "r", encoding="utf-8") as f:
            existing_counts = load(f)

    existing_counts.update(to_add)

    with open(dict_path, "w", encoding="utf-8") as f:
        dump(existing_counts, f, indent=4, ensure_ascii=False)


def analyze_relative_word_freq(mode: str, n: int, chart_path=None):
    set_option("display.float_format", "{:.6f}".format)
    data = get_relative_freq_table(mode=mode, n=n)

    print(data)

    if chart_path is not None:
        dyn_width = max(10, int(n * 0.8))
        data.plot(
            x="word",
            y=["frequency in the article", "frequency in the wiki language"],
            kind="bar",
            color=["royalblue", "indianred"],
            title=(f"Top {n} words by relative frequency "
                   f"(sorted by appearance in {mode})"),
            xlabel="",
            ylabel="Frequency",
            legend=True,
            figsize=(dyn_width, 6),
            rot=0
        )
        plt.savefig(chart_path)


def get_relative_freq_table(mode: str, n: int) -> DataFrame:
    word_counts = {}

    if dict_path.exists():
        with open(dict_path, "r", encoding="utf-8") as f:
            word_counts = load(f)

    total = sum(word_counts.values())
    if total == 0:
        total = 1

    for word, count in word_counts.items():
        word_counts[word] = count / total

    if mode == "article":
        df = DataFrame(
            list(word_counts.items()),
            columns=["word", "frequency in the article"]
        )

        df_sorted = df.sort_values(
            by="frequency in the article",
            ascending=False
        ).head(n)

        df_sorted["frequency in the wiki language"] = df_sorted["word"].apply(
            lambda w: word_frequency(w, "en")
        )

        return df_sorted.reset_index(drop=True)
    elif mode == "language":
        top_words = top_n_list("en", n)

        df = DataFrame({"word": top_words})
        df["frequency in the wiki language"] = df["word"].apply(
            lambda w: word_frequency(w, "en")
        )

        df["frequency in the article"] = df["word"].apply(
            lambda w: word_counts.get(w, 0)
        )

        return df.reset_index(drop=True)
    else:
        # should not happen
        return DataFrame()


def auto_count_words(start_phrase: str, depth: int, wait: float):
    from wiki_scraper.scraper import Scraper

    start_phrase = format_phrase(start_phrase)
    begin_url = get_url_from_phrase(start_phrase)

    visited = set()
    to_visit = [(begin_url, 0)]

    while len(to_visit) > 0:
        current_url, current_depth = to_visit.pop(0)

        if (current_url in visited) or (current_depth > depth) \
                or (not current_url.startswith(BULBAPEDIA_URL)):
            continue

        visited.add(current_url)

        scraper = Scraper(phrase=get_phrase_from_url(current_url))

        article = scraper.scrape()

        word_counts = article.count_words()
        update_word_counts(word_counts)

        if article.container is not None:
            for link in article.container.find_all("a", href=True):
                href = link['href']
                if href.startswith("/wiki/") and ':' not in href:
                    # This prefix is already in `BULBAPEDIA_URL`
                    full_url = get_url_from_phrase(href)
                    to_visit.append((full_url, current_depth + 1))

        time.sleep(wait)


def get_url_from_phrase(phrase: str) -> str:
    phrase = phrase.removeprefix("/wiki/")
    return BULBAPEDIA_URL + phrase


def get_phrase_from_url(url: str) -> str:
    return url.removeprefix(BULBAPEDIA_URL).removeprefix("/wiki/")


def format_phrase(phrase: str) -> str | None:
    if phrase is None or len(phrase) == 0:
        return phrase

    phrase = phrase[0].upper() + phrase[1:]
    return phrase.replace(" ", "_")
