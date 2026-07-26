# tests/test_utils.py
# Unit tests for functions in `utils.py`:
# 1. updating `word_counts.json` file,
import unittest
from wiki_scraper.utils import update_word_counts
from wiki_scraper.utils import dict_path
from wiki_scraper.utils import analyze_relative_word_freq
import os
import json


class TestUpdatingWordCounts(unittest.TestCase):
    def setUp(self):
        # Ensure the dict file does not exist before each test
        if dict_path.exists():
            os.remove(dict_path)

    def test_update_word_counts_simple(self):
        to_add = {"test": 3, "word": 5}
        update_word_counts(to_add)

        self.assertTrue(dict_path.exists())
        with open(dict_path, "r", encoding="utf-8") as f:
            data = json.load(f)

        self.assertEqual(data, to_add)

    def test_update_word_counts_existing_file(self):
        initial_data = {"hello": 2, "world": 4}
        with open(dict_path, "w", encoding="utf-8") as f:
            json.dump(initial_data, f)

        to_add = {"world": 3, "new": 1}
        update_word_counts(to_add)

        with open(dict_path, "r", encoding="utf-8") as f:
            data = json.load(f)

        expected_data = {"hello": 2, "world": 3, "new": 1}
        self.assertEqual(expected_data, data)

    def test_update_word_counts_multiple_calls(self):
        # 1
        update_word_counts({"hello": 2, "world": 1})
        with open(dict_path, "r", encoding="utf-8") as f:
            data1 = json.load(f)
        self.assertEqual({"hello": 2, "world": 1}, data1)

        # 2
        update_word_counts({"hello": 5, "python": 3})
        with open(dict_path, "r", encoding="utf-8") as f:
            data2 = json.load(f)

        expected_data = {"hello": 5, "world": 1, "python": 3}
        self.assertEqual(expected_data, data2)

    def test_update_word_counts_polish_characters(self):
        to_add = {"zażółć": 2, "gęślą": 1, "łódź": 3, "Racibórz": 1}
        update_word_counts(to_add)

        self.assertTrue(dict_path.exists())
        with open(dict_path, "r", encoding="utf-8") as f:
            data = json.load(f)

        self.assertEqual(to_add, data)


class TestAnalyzeRelativeWordFreq(unittest.TestCase):
    def test_analyze_relative_word_freq_article(self):
        analyze_relative_word_freq(mode="article", n=10, chart_path="chart_a.png")

    def test_analyze_rel_word_freq_language(self):
        analyze_relative_word_freq(mode="language", n=10, chart_path="chart_l.png")


if __name__ == "__main__":
    unittest.main()
