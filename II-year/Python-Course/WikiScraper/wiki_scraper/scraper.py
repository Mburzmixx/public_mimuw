# Module containing implementation of class `Scraper`.
# This class is responsible for scraping content from an appropriate file or a web-page.
import requests
import time
from wiki_scraper.article import Article
from wiki_scraper.exceptions import ArticleNotFound
from wiki_scraper.utils import BULBAPEDIA_URL


class Scraper:
    num_attempts = 5
    requests_timeout = 10
    wait_seconds = 2
    headers = {
        "User-Agent": (
            "wiki_scraper_bot/1.0 "
            "(contact: mburza@student.uw.edu.pl)"
        )
    }

    def __init__(self, phrase: str, base_url=BULBAPEDIA_URL,
                 use_local_file=False):
        # if `use_local_file=True`, then path to local file
        # should be given in `base_url`.
        # In such case, encoding is assumed to be `utf-8`.
        self.base_url = base_url
        self.phrase = phrase
        self.use_local_file = use_local_file

    def scrape(self) -> Article:
        if self.use_local_file:
            return self.scrape_from_file()
        else:
            return self.scrape_from_web()

    def scrape_from_file(self) -> Article:
        try:
            with open(self.base_url, "r", encoding="utf-8") as f:
                html_content = f.read()
            return Article(html_content=html_content, phrase=self.phrase)
        except FileNotFoundError:
            raise ArticleNotFound(
                f"Local file '{self.base_url}' not found."
            )
        except UnicodeDecodeError:
            raise ArticleNotFound(
                f"Cannot decode file '{self.base_url}' with 'utf-8' encoding."
            )
        except PermissionError:
            raise ArticleNotFound(
                f"Permission denied to read file '{self.base_url}'."
            )

    def scrape_from_web(self) -> Article:
        if self.phrase is None:
            raise ArticleNotFound("Phrase is None.")

        self.base_url = self.base_url.rstrip("/")
        url = f"{self.base_url}/{self.phrase.replace(' ', '_')}"

        response = None
        for attempt in range(self.num_attempts):
            try:
                response = requests.get(url, headers=self.headers,
                                        timeout=self.requests_timeout)
                # Retry only on 429 -- Too Many Requests
                if response.status_code == 429:
                    retry_after = response.headers.get("Retry-After")

                    wait = (
                        float(retry_after)
                        if retry_after and retry_after.isdigit()
                        else self.wait_seconds
                    )
                    time.sleep(wait)
                    continue

                response.raise_for_status()
                break
            except requests.RequestException:
                response = None
                time.sleep(self.wait_seconds)

        if response is None:
            raise ArticleNotFound(
                f"Failed to fetch '{url}' after {self.num_attempts} attempts."
            )

        # Map other error codes to ArticleNotFound explicitly
        if response.status_code >= 400:
            raise ArticleNotFound(
                f"HTTP {response.status_code} when fetching '{url}'."
            )

        html_content = response.text
        return Article(html_content=html_content, phrase=self.phrase)
