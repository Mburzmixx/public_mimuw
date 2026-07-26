# Module containing implementation of class `Article`.
# This class represents an article
# and is responsible for parsing scrapped content.
from collections import Counter
from bs4 import BeautifulSoup
from pandas import DataFrame, read_html
from re import findall, sub
from io import StringIO


class Article:
    def __init__(self, html_content, phrase: str):
        self.html_content = html_content
        self.phrase = phrase
        self._parsed_content = BeautifulSoup(self.html_content, "html.parser")

        temp_container = self._parsed_content.find(
            "div", class_="mw-content-ltr mw-parser-output"
        )

        if temp_container is None:
            self.container = None
            return

        # Cleans unwanted tags from the container
        # (stored as nodes in the BeautifulSoup tree)
        for tag in temp_container.find_all(["style", "script"]):
            tag.decompose()

        self.container = temp_container

    def get_first_paragraph(self) -> str:
        if self.container is not None:
            paragraph = self.container.find("p")
            if paragraph is not None:
                text = paragraph.get_text(separator=" ", strip=True)

                # Remove spaces before punctuation marks
                return sub(r'\s+([.,!?;:)])', r'\1', text)
        return ""

    def get_table_by_index(self, index: int) -> DataFrame:
        if self.container is None:
            return DataFrame()

        tables = self.container.find_all("table")

        if index < 1 or index > len(tables):
            return DataFrame()

        nth_table = tables[index - 1]
        # function from `pandas`
        df = read_html(StringIO(str(nth_table)))[0]
        return df

    def count_words(self) -> dict[str, int]:
        if self.container is None:
            return {}

        text = self.container.get_text(separator=" ", strip=True)

        # regex looking for words, allowing internal apostrophes and e-acute
        pattern = r"[A-Za-zÉé]+(?:'[A-Za-zÉé]+)*"
        words = findall(pattern, text.lower())

        # creates needed dictionary
        return dict(Counter(words))
        