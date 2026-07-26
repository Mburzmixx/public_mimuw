# Main module.
from wiki_scraper.cli import get_args
from wiki_scraper.controller import Controller
from wiki_scraper.exceptions import ArticleNotFound
from wiki_scraper.utils import OK


def main(argv=None):
    args = get_args() if argv is None else get_args(argv)
    controller = Controller(args)
    try:
        if controller.run() == OK:
            print("\nOK: wiki_scraper exited successfully!")
    except ArticleNotFound as e:
        print("\nERROR: Article not found. Message:\n", str(e))


if __name__ == "__main__":
    main()
