from wiki_scraper.scraper import Scraper
from json import dump, load
from pathlib import Path

phrase_big_article = [
    "Eevee_(Pokémon)",
    "Pikachu_(Pokémon)",
    "Legendary_Pokémon",
    "Villainous_team"]
phrase_low_conf_score = "Fairy_(type)"

path_big_article = "wiki_big_article_wc.json"
path_low_conf_score = "wiki_low_conf_score_wc.json"


def update_dict_in_file(d: dict, path: str):
    path = Path(path)
    data = {}

    if path.exists():
        with open(path, "r", encoding="utf-8") as f:
            data = load(f)

    data.update(d)
    with open(path, "w", encoding="utf-8") as f:
        dump(data, f, ensure_ascii=False, indent=4)


def main():
    from os import remove

    if Path(path_big_article).exists():
        remove(path_big_article)
    
    if Path(path_low_conf_score).exists():
        remove(path_low_conf_score)

    for phrase in phrase_big_article:
        scraper1 = Scraper(phrase=phrase)
        article1 = scraper1.scrape()
        wiki_big_article_wc = article1.count_words()
        update_dict_in_file(
            d=wiki_big_article_wc,
            path=path_big_article
        )

    scraper2 = Scraper(phrase=phrase_low_conf_score)
    article2 = scraper2.scrape()
    low_conf_score_article_wc = article2.count_words()
    update_dict_in_file(
        d=low_conf_score_article_wc,
        path=path_low_conf_score
    )

    print("Successfully wrote word count dictionaries to files!")


if __name__ == "__main__":
    main()
