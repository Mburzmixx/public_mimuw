# pythonWikiScraper
This is a repository for the final project for the Python programming course, 2nd year MIM UW.

## Data Source & License

This project uses data from [Bulbapedia](https://bulbapedia.bulbagarden.net/wiki/Main_Page), which is licensed under 
[CC BY-NC-SA License](https://creativecommons.org/licenses/by-nc-sa/4.0/).

**Attribution:** Data sourced from various Bulbapedia articles

**License Requirements:**
- Non-commercial use only
- Share derivatives under the same license
- Provide appropriate attribution


## Usage:

**Required modules:**
- wordfreq
- pandas
- bs4
- lxml
- requests
- matplotlib

Run the CLI after activating your Python environment:

- `python wiki_scraper.py --summary "SEARCH PHRASE"`
	- Fetches the article and prints the first paragraph (content only).
- `python wiki_scraper.py --table "SEARCH PHRASE" --number N`
	- Extracts the N‑th table, saves it to `SEARCH_PHRASE.csv`, and prints value counts.
- `python wiki_scraper.py --count-words "SEARCH PHRASE"`
	- Counts words in the article and updates `wiki_scraper/word-counts.json`.
- `python wiki_scraper.py --analyze-relative-word-frequency --mode "article|language" --count N [--chart "path/to/chart.png"]`
	- Compares article frequencies with language frequencies and optionally saves a bar chart.
- `python wiki_scraper.py --auto-count-words "START PHRASE" --depth N --wait T`
	- Crawls links up to depth `N`, counts words, and waits `T` seconds between requests.

Notes:
- Phrases use spaces or underscores; the scraper converts them to wiki URLs.
- The project targets Bulbapedia and respects its CC BY‑NC‑SA license.

## Jupyter Notebook

The language analysis is in `language_analysis.ipynb` and is intended to be run in Google Colab.

1. Open https://colab.research.google.com/
2. Upload `language_analysis.ipynb`.
3. Before running the notebook, generate the input dictionaries by running:
	 - `python get_dicts_to_analysis.py`
4. Upload the produced JSON files in Colab when prompted:
	 - `wiki_big_article_wc.json`
	 - `wiki_low_conf_score_wc.json`
