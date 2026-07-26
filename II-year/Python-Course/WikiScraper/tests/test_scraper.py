# tests/test_scraper.py
# Unit tests for class `Scraper`:
# 1. local file scraping,
# 2. mocking `requests.get`, checking all most important codes.
import unittest
from pathlib import Path
from unittest.mock import patch, Mock
import requests

from wiki_scraper.scraper import Scraper
from wiki_scraper.article import Article
from wiki_scraper.exceptions import ArticleNotFound


class TestScraperLocal(unittest.TestCase):
    def setUp(self):
        # Resolve path to sample HTML shipped with the repo
        repo_root = Path(__file__).resolve().parent.parent
        self.sample_path = repo_root / "tests" / "sample_data" / "team_rocket.html"

    def test_scrape_from_file_success(self):
        scraper = Scraper(
            base_url=str(self.sample_path),
            phrase="Team Rocket",
            use_local_file=True
        )
        article = scraper.scrape()

        self.assertIsInstance(article, Article)
        self.assertEqual("Team Rocket", article.phrase)
        self.assertIn("Team Rocket", article.html_content)

    def test_scrape_from_file_not_found(self):
        missing_file = self.sample_path.parent / "missing.html"
        scraper = Scraper(
            base_url=str(missing_file),
            phrase="Anything",
            use_local_file=True
        )

        with self.assertRaises(ArticleNotFound):
            scraper.scrape()


class TestScraperMockGet(unittest.TestCase):
    @patch("wiki_scraper.scraper.requests.get")
    def test_scrape_from_web_success(self, mock_get):
        # Mock response object
        mock_200 = Mock()
        mock_200.status_code = 200
        mock_200.text = """
            <html>
                <body>
                    <div class='mw-content-ltr mw-parser-output'>
                        <p> Test paragraph -- Generations. </p>
                    </div>
                </body>
            </html>
            """
        mock_get.return_value = mock_200

        scraper = Scraper(
            base_url="https://bulbapedia.bulbagarden.net/wiki/",
            phrase="Generation",
            use_local_file=False
        )

        article = scraper.scrape()

        self.assertIsInstance(article, Article)
        self.assertEqual("Generation", article.phrase)
        self.assertIn("Test paragraph -- Generations.", article.html_content)

        mock_get.assert_called_once_with(
            "https://bulbapedia.bulbagarden.net/wiki/Generation",
            headers=Scraper.headers,
            timeout=Scraper.requests_timeout
        )

    @patch("wiki_scraper.scraper.requests.get")
    def test_scrape_from_web_not_found(self, mock_get):
        # Mock response object for 404
        mock_404 = Mock()
        mock_404.status_code = 404
        mock_404.raise_for_status.side_effect = requests.HTTPError(
            "404 Client Error: Not Found for url"
        )
        mock_get.return_value = mock_404

        scraper = Scraper(
            base_url="https://bulbapedia.bulbagarden.net/wiki/",
            phrase="NonExistentArticle",
            use_local_file=False
        )

        with self.assertRaises(ArticleNotFound):
            scraper.scrape()

        self.assertEqual(Scraper.num_attempts, mock_get.call_count)

    @patch("wiki_scraper.scraper.time.sleep")
    @patch("wiki_scraper.scraper.requests.get")
    def test_scrape_from_web_retry(self, mock_get, mock_sleep):
        mock_429 = Mock()
        mock_429.status_code = 429
        mock_429.headers = {"Retry-After": "2"}

        mock_200 = Mock()
        mock_200.status_code = 200
        mock_200.text = """
            <html>
                <body>
                    <div class='mw-content-ltr mw-parser-output'>
                        <p> Test paragraph after retry. </p>
                    </div>
                </body>
            </html>
            """

        mock_get.side_effect = [mock_429, mock_429, mock_200]

        scraper = Scraper(
            base_url="https://bulbapedia.bulbagarden.net/wiki/",
            phrase="Generation",
            use_local_file=False
        )
        article = scraper.scrape()

        self.assertIsInstance(article, Article)
        self.assertEqual(3, mock_get.call_count)
        self.assertEqual(2, mock_sleep.call_count)
        self.assertEqual("Generation", article.phrase)
        mock_get.assert_called_with(
            "https://bulbapedia.bulbagarden.net/wiki/Generation",
            headers=Scraper.headers,
            timeout=Scraper.requests_timeout
        )


if __name__ == "__main__":
    unittest.main()
