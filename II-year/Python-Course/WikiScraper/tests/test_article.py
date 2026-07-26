# tests/test_article_reading_words.py
# Unit tests for class `Article`:
# 1. focused on checking how it reads words,
# 2. focused on reading tables,
# 3. focused on counting words.
import unittest
from pathlib import Path
from pandas import Series
from wiki_scraper.article import Article
from wiki_scraper.scraper import Scraper


class TestArticleWordReading(unittest.TestCase):
    def setUp(self):
        # Resolve path to sample HTML shipped with the repo
        repo_root = Path(__file__).resolve().parent.parent
        self.simple_path = [repo_root / "tests" / "sample_data" /
                            f"simple_article_{i}.html" for i in range(4)]

    def test_empty_article_mock(self):
        article = Article("", "")

        expected_str = ""
        actual_str = article.get_first_paragraph()

        self.assertEqual(expected_str, actual_str)

    def test_empty_article_locally(self):
        scraper = Scraper(
            base_url=str(self.simple_path[0]),
            phrase="Test0",
            use_local_file=True
        )
        article = scraper.scrape()

        expected_str = ""
        actual_str = article.get_first_paragraph()

        self.assertEqual(expected_str, actual_str)

    def test_reading_first_paragraph_easy(self):
        scraper = Scraper(
            base_url=str(self.simple_path[1]),
            phrase="Test1",
            use_local_file=True
        )
        article = scraper.scrape()

        expected_str = "This is the first paragraph. It contains some text for testing."
        actual_str = article.get_first_paragraph()

        self.assertEqual(expected_str, actual_str)

    def test_reading_first_paragraph_sep(self):
        scraper = Scraper(
            base_url=str(self.simple_path[2]),
            phrase="Test2",
            use_local_file=True
        )
        article = scraper.scrape()

        expected_str = ("This is the first paragraph. It contains some text for testing. "
                        "This time it is tricky")
        actual_str = article.get_first_paragraph()

        self.assertEqual(expected_str, actual_str)

    def test_article_without_paragraphs(self):
        scraper = Scraper(
            base_url=str(self.simple_path[3]),
            phrase="Test3",
            use_local_file=True
        )
        article = scraper.scrape()

        expected_str = ""
        actual_str = article.get_first_paragraph()

        self.assertEqual(expected_str, actual_str)


class TestArticleReadingTables(unittest.TestCase):
    def setUp(self):
        # Resolve path to sample HTML shipped with the repo
        repo_root = Path(__file__).resolve().parent.parent
        self.simple_path = [repo_root / "tests" / "sample_data" /
                            f"table_simple_{i}.html" for i in range(4)]

        self.mid_path = [repo_root / "tests" / "sample_data" /
                         f"table_mid_{i}.html" for i in range(1)]

    def test_simple_table(self):
        scraper = Scraper(
            base_url=str(self.simple_path[0]),
            phrase="Test0",
            use_local_file=True
        )
        article = scraper.scrape()
        df = article.get_table_by_index(1)

        self.assertEqual((3, 3), df.shape)
        self.assertIn("Type", df.columns)
        self.assertIn("Color", df.columns)
        self.assertIn("Example", df.columns)

        self.assertEqual("Fire", df.iloc[0, 0])
        self.assertEqual("Water", df.iloc[1, 0])
        self.assertEqual("Grass", df.iloc[2, 0])

    def test_table_index_not_one(self):
        scraper = Scraper(
            base_url=str(self.simple_path[1]),
            phrase="Test1",
            use_local_file=True
        )
        article = scraper.scrape()
        df = article.get_table_by_index(2)

        self.assertEqual((2, 3), df.shape)
        self.assertIn("Generation", df.columns)

    def test_bigger_table(self):
        scraper = Scraper(
            base_url=str(self.simple_path[2]),
            phrase="Test2",
            use_local_file=True
        )
        article = scraper.scrape()
        df = article.get_table_by_index(1)

        self.assertEqual((3, 4), df.shape)
        self.assertIn("Super Effective Against", df.columns)

        self.assertEqual("Fire", df.iloc[0, 0])
        self.assertIn("Grass", df.iloc[0, 1])

    def test_no_tables(self):
        scraper = Scraper(
            base_url=str(self.simple_path[3]),
            phrase="Test3",
            use_local_file=True
        )
        article = scraper.scrape()
        df = article.get_table_by_index(1)

        self.assertTrue(df.empty)

    def test_detection_of_headers(self):
        scraper = Scraper(
            base_url=str(self.mid_path[0]),
            phrase="Test4",
            use_local_file=True
        )
        article = scraper.scrape()
        df = article.get_table_by_index(1)

        self.assertEqual((6, 2), df.shape)
        self.assertIn("Type effectiveness", df.columns)
        self.assertIn("Multiplier", df.columns)

        self.assertEqual("Doubly super effective", df.iloc[0, 0])
        self.assertEqual("×2.56", df.iloc[0, 1])

        self.assertIn("Triply resisted", df.iloc[5, 0])
        self.assertEqual("×0.244140625", df.iloc[5, 1])


class TestArticleCountingWords(unittest.TestCase):
    def setUp(self):
        repo_root = Path(__file__).resolve().parent.parent
        self.simple_table_path = [repo_root / "tests" / "sample_data" /
                                  f"table_simple_{i}.html" for i in range(4)]
        self.mid_table_path = [repo_root / "tests" / "sample_data" /
                               f"table_mid_{i}.html" for i in range(1)]

    def test_count_words_simple_table(self):
        scraper = Scraper(
            base_url=str(self.simple_table_path[0]),
            phrase="TableTest0",
            use_local_file=True
        )
        article = scraper.scrape()
        word_counts = article.count_words()

        self.assertIn("fire", word_counts)
        self.assertIn("water", word_counts)
        self.assertIn("grass", word_counts)
        self.assertEqual(1, word_counts["fire"])
        self.assertEqual(1, word_counts["water"])
        self.assertEqual(1, word_counts["grass"])
        self.assertEqual(1, word_counts["type"])

    def test_count_words_two_tables(self):
        scraper = Scraper(
            base_url=str(self.simple_table_path[1]),
            phrase="TableTest1",
            use_local_file=True
        )
        article = scraper.scrape()
        word_counts = article.count_words()

        self.assertIn("gen", word_counts)
        self.assertIn("generation", word_counts)
        self.assertIn("year", word_counts)
        self.assertEqual(4, word_counts["gen"])
        self.assertEqual(1, word_counts["generation"])
        self.assertEqual(1, word_counts["year"])
        self.assertEqual(1, word_counts["overview"])

    def test_count_words_complex_table(self):
        scraper = Scraper(
            base_url=str(self.simple_table_path[2]),
            phrase="TableTest2",
            use_local_file=True
        )
        article = scraper.scrape()
        word_counts = article.count_words()

        self.assertIn("type", word_counts)
        self.assertIn("fire", word_counts)
        self.assertIn("water", word_counts)
        self.assertIn("effectiveness", word_counts)
        self.assertEqual(4, word_counts["fire"])
        self.assertEqual(5, word_counts["water"])
        self.assertEqual(5, word_counts["grass"])
        self.assertEqual(3, word_counts["type"])
        self.assertGreater(len(word_counts), 10)

    def test_count_words_no_tables(self):
        scraper = Scraper(
            base_url=str(self.simple_table_path[3]),
            phrase="TableTest3",
            use_local_file=True
        )
        article = scraper.scrape()
        word_counts = article.count_words()

        self.assertIn("text", word_counts)
        self.assertIn("article", word_counts)
        self.assertIn("tables", word_counts)
        self.assertEqual(1, word_counts["text"])
        self.assertEqual(1, word_counts["article"])
        self.assertEqual(2, word_counts["tables"])
        self.assertEqual(1, word_counts["just"])
        self.assertGreater(len(word_counts), 5)

    def test_count_words_mid_table(self):
        scraper = Scraper(
            base_url=str(self.mid_table_path[0]),
            phrase="BulbaTest",
            use_local_file=True
        )
        article = scraper.scrape()
        word_counts = article.count_words()

        self.assertIn("type", word_counts)
        self.assertIn("effectiveness", word_counts)
        self.assertIn("doubly", word_counts)
        self.assertIn("super", word_counts)
        self.assertIn("effective", word_counts)
        self.assertEqual(2, word_counts["effective"])
        self.assertEqual(2, word_counts["doubly"])
        self.assertEqual(3, word_counts["resisted"])
        self.assertEqual(3, word_counts["type"])


class TestPrintingValueFrequency(unittest.TestCase):
    def setUp(self):
        repo_root = Path(__file__).resolve().parent.parent
        self.simple_table_path = [repo_root / "tests" / "sample_data" /
                                  f"table_simple_{i}.html" for i in range(4)]
        self.mid_table_path = [repo_root / "tests" / "sample_data" /
                               f"table_mid_{i}.html" for i in range(1)]

    def test_count_words_in_a_complex_table(self):
        scraper = Scraper(
            base_url=str(self.simple_table_path[2]),
            phrase="SimpleTableTest",
            use_local_file=True
        )
        article = scraper.scrape()
        df = article.get_table_by_index(index=1)

        flattened_df = Series(df.values.ravel()).value_counts()

        self.assertEqual(flattened_df.loc["Water, Ground, Rock"], 2)
        self.assertEqual(flattened_df.loc["Fire"], 1)
        self.assertEqual(flattened_df.loc["Water"], 1)
        self.assertEqual(flattened_df.loc["Grass"], 1)
        self.assertEqual(flattened_df.sum(), 12)

    def test_value_counts_mid_table(self):
        scraper = Scraper(
            base_url=str(self.mid_table_path[0]),
            phrase="MidTableTest",
            use_local_file=True
        )
        article = scraper.scrape()
        df = article.get_table_by_index(index=1)

        flattened_df = Series(df.values.ravel()).value_counts()

        # Ensure expected entries are present exactly once
        self.assertEqual(flattened_df.loc["Doubly super effective"], 1)
        self.assertEqual(flattened_df.loc["Super effective"], 1)
        self.assertEqual(flattened_df.loc["Neutral"], 1)
        self.assertEqual(flattened_df.loc["Resisted"], 1)
        self.assertEqual(flattened_df.loc["Doubly resisted"], 1)
        # Triply resisted text includes the asterisk from the span
        self.assertEqual(flattened_df.loc["Triply resisted*"], 1)
        self.assertEqual(flattened_df.loc["×2.56"], 1)
        self.assertEqual(flattened_df.loc["×1.6"], 1)
        self.assertEqual(flattened_df.loc["×1"], 1)
        self.assertEqual(flattened_df.loc["×0.625"], 1)
        self.assertEqual(flattened_df.loc["×0.390625"], 1)
        self.assertEqual(flattened_df.loc["×0.244140625"], 1)
        # Total cells: 6 rows * 2 columns
        self.assertEqual(flattened_df.sum(), 12)


if __name__ == "__main__":
    unittest.main()
