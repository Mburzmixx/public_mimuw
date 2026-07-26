# Module containing implementation of class `Controller`,
# which manages the flow of the program.
from pandas import Series
from wiki_scraper.scraper import Scraper
from wiki_scraper.utils import (OK, update_word_counts,
                                analyze_relative_word_freq, auto_count_words)


class Controller:
    def __init__(self, args):
        self.article = None
        self.args = args
        self.phrase = args.phrase

    def run(self):
        handlers = {
            "summary": self._handle_summary,
            "table": self._handle_table,
            "count-words": self._handle_count_words,
            "analyze-relative-word-frequency": self._handle_relative_word_freq,
            "auto-count-words": self._handle_auto_count_words,
        }

        handlers[self.args.cmd]()
        return OK

    def _handle_summary(self):
        self._ensure_article()
        print(self.article.get_first_paragraph())

    def _handle_table(self):
        self._ensure_article()
        index = self.args.number

        df = self.article.get_table_by_index(index=index)
        path = self.phrase + ".csv"
        df.to_csv(path, index=False)
        flattened_df = Series(df.values.ravel()).value_counts()
        print(flattened_df)

    def _handle_count_words(self):
        self._ensure_article()
        to_add = self.article.count_words()
        update_word_counts(to_add)

    def _handle_relative_word_freq(self):
        mode = self.args.mode
        n = self.args.count
        chart_path = self.args.chart
        analyze_relative_word_freq(mode=mode, n=n, chart_path=chart_path)

    def _handle_auto_count_words(self):
        start_phrase = self.phrase
        depth = self.args.depth
        wait = self.args.wait
        auto_count_words(start_phrase=start_phrase, depth=depth, wait=wait)

    def _ensure_article(self):
        # Maybe without if, so as article will be refreshed each time?
        # Then checking if article.phrase == self.phrase
        # What if there should be two phrases in self.phrase?
        if self.article is None and self.phrase is not None:
            scraper = Scraper(phrase=self.phrase)
            self.article = scraper.scrape()
