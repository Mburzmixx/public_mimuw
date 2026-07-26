# integration test for wiki_scraper.py
from wiki_scraper.main import main as wiki_scraper_main
from wiki_scraper.scraper import Scraper
from wiki_scraper.utils import dict_path


def main():
    scraper = Scraper(
        phrase="Villainous Team",
        base_url="tests/sample_data/villainous_team.html",
        use_local_file=True
    )
    article = scraper.scrape()
    first_paragraph = article.get_first_paragraph()
    try:
        assert first_paragraph == (
            "A villainous team (Japanese: 悪の組織 evil organization) "
            "is a group of antagonists with a goal in opposition "
            "to the player and other friendly characters. "
            "Each generation of the core series games features "
            "a villainous team as a major plot element."
        )
        print(f"✓ summary test passed!")
    except AssertionError as e:
        print(f"✗ summary test failed: {e}")
        exit(1)

    # Test count_words
    word_counts = article.count_words()
    try:
        assert isinstance(word_counts, dict)
        assert "team" in word_counts
        assert "villainous" in word_counts
        assert word_counts["team"] >= 3
        assert len(word_counts) > 20
        print(f"✓ count-words test passed, {len(word_counts)} words found!")
    except AssertionError as e:
        print(f"✗ count-words test failed: {e}.")
        exit(1)


def online_test():
    # --- SUMMARY ---
    test_args = [
        "wiki_scraper",
        "--summary", "Team Rocket"
    ]
    wiki_scraper_main(test_args)
    print(f"✓ online summary test passed!")

    # --- NON-EXISTENT ARTICLE ---
    test_args = [
        "wiki_scraper",
        "--summary", "NonExistentArticle"
    ]
    wiki_scraper_main(test_args)
    # should print an error message
    print(f"✓ online non-existent article test passed!")

    # --- TABLE ---
    test_args = [
        "wiki_scraper",
        "--table", "Type",
        "--number", "2"
    ]
    wiki_scraper_main(test_args)
    # should print the 2nd table from the "Type" article,
    # phrase "These matchups are suitable for Generation VI onward."
    # gets counted 20 times, as it takes 20 cells in the 2nd table.
    print(f"✓ online table test passed!")

    # --- COUNT WORDS ---
    from os import remove

    if dict_path.exists():
        remove(dict_path)

    test_args = [
        "wiki_scraper",
        "--count-words", "the Electric Tale of Pikachu"
    ]
    wiki_scraper_main(test_args)
    print(f"✓ online count-words test passed!")

    # --- ANALYZE RELATIVE WORD FREQUENCY ---
    test_args = [
        "wiki_scraper",
        "--analyze-relative-word-frequency",
        "--mode", "article",
        "--count", "25",
        "--chart", "relative_freq_chart.png"
    ]
    wiki_scraper_main(test_args)
    print(f"✓ online analyze-relative-word-frequency test passed!")

    # --- AUTO COUNT WORDS ---
    test_args = [
        "wiki_scraper",
        "--auto-count-words", "Tropical_Wind",
        "--depth", "1",
        "--wait", "0"
    ]
    wiki_scraper_main(test_args)
    print(f"✓ online auto-count-words test passed!")


if __name__ == "__main__":
    main()
    online_test()
