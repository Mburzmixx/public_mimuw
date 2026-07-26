# Module responsible for command-line-interface.
# Here I will parse arguments.
import sys
import argparse
from wiki_scraper.utils import format_phrase

parser_description = ("USAGE\n"
                      "You should call with one of the following options:\n"
                      "--summary `your_phrase`\n"
                      "--table `your_phrase` --number n\n"
                      "--count-words `your_phrase`\n"
                      "--analyze-relative-word-frequency"
                      " --mode [`article`, `language`] --count n "
                      "[-- chart `path.png`]\n"
                      "--auto-count-words `your_begin_phrase`"
                      " --depth n --wait t\n\n"
                      "Other use cases won't be served.\n")


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=parser_description,
        # enables manual formating (instead of automatic)
        formatter_class=argparse.RawDescriptionHelpFormatter
    )

    sub = parser.add_subparsers(dest="cmd", required=True)

    # SUMMARY
    p_summary = sub.add_parser("summary", help="Get article summary.")
    p_summary.add_argument(
        "phrase",
        type=alphanumeric_phrase,
        help="Phrase to search for."
    )

    # TABLE
    p_table = sub.add_parser("table", help="Get table by index.")
    p_table.add_argument(
        "phrase",
        type=alphanumeric_phrase,
        help="Phrase to search for."
    )
    p_table.add_argument(
        "--number",
        type=int,
        help="Index of the table to retrieve.",
        required=True
    )

    # COUNT WORDS
    p_count_words = sub.add_parser(
        "count-words",
        help="Count words in the article."
    )
    p_count_words.add_argument(
        "phrase",
        type=alphanumeric_phrase,
        help="Phrase to search for.",
    )

    # ANALYZE RELATIVE WORD FREQUENCY
    p_analyze_relative_word_frequency = sub.add_parser(
        "analyze-relative-word-frequency",
        help="Analyze relative word frequency."
    )
    p_analyze_relative_word_frequency.add_argument(
        "--mode",
        choices=["article", "language"],
        required=True,
        help="Mode of analysis."
    )
    p_analyze_relative_word_frequency.add_argument(
        "--count",
        type=int,
        required=True,
        help="Number of top words to analyze."
    )
    p_analyze_relative_word_frequency.add_argument(
        "--chart",
        type=str,
        required=False,
        help="Path to save the generated chart."
    )

    # AUTO COUNT WORDS
    p_auto_count_words = sub.add_parser(
        "auto-count-words",
        help="Automatically count words starting from a phrase."
    )
    p_auto_count_words.add_argument(
        "phrase",
        type=alphanumeric_phrase,
        help="Starting phrase.",
    )
    p_auto_count_words.add_argument(
        "--depth",
        type=int,
        required=True,
        help="Depth of traversal."
    )
    p_auto_count_words.add_argument(
        "--wait",
        type=float,
        required=True,
        help="Wait time between requests."
    )

    return parser.parse_args(argv)


def alphanumeric_phrase(phrase: str) -> str:
    chars_to_remove = [' ', '_', '-', '(', ')', 'é', '"', "'", '\'']

    clean = phrase
    for ch in chars_to_remove:
        clean = clean.replace(ch, "")

    if not clean.isalnum():
        raise argparse.ArgumentTypeError(
            f"Phrase '{phrase}' is not alphanumeric."
        )

    if len(phrase) == 0:
        raise argparse.ArgumentTypeError("Phrase cannot be empty.")

    return phrase


def normalize_legacy_flags(argv: list[str]) -> list[str]:
    mapping = {
        "--summary": "summary",
        "--table": "table",
        "--count-words": "count-words",
        "--analyze-relative-word-frequency": "analyze-relative-word-frequency",
        "--auto-count-words": "auto-count-words"
    }

    if argv and (argv[0] in mapping):
        return [mapping[argv[0]]] + argv[1:]
    return argv


def get_args(argv: list[str] | None = None) -> argparse.Namespace:
    raw_argv = sys.argv[1:] if argv is None else argv[1:]
    _argv = normalize_legacy_flags(raw_argv)

    args = parse_args(_argv)

    if not hasattr(args, "phrase"):
        args.phrase = ""

    args.phrase = format_phrase(args.phrase)
    return args
