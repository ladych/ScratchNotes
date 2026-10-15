#!/usr/bin/env python3
"""Checks the translation files of Scratch Notes.

Collects the English texts of i18n::tr(), i18n::trJuce() and i18n::marked() in src/ and checks every file
resources/Languages/<code>.txt:
- every text of the program has a translation,
- no translation is left over for a text the program no longer has (entries below the line "# JUCE" are
  JUCE's own texts and exempt),
- no text is translated twice,
- a translation has the same placeholders %1 ... %9 as its text.

Usage: tools/check-translations.py [repository root]
Exits with 1 and lists the problems if a check fails.
"""

import pathlib
import re
import sys

CALL = re.compile(r'\b(?:tr|trJuce|marked)\(\s*((?:"(?:[^"\\]|\\.)*"\s*)+)')
LITERAL = re.compile(r'"((?:[^"\\]|\\.)*)"')
PLACEHOLDER = re.compile(r"%[1-9]")
JUCE_SECTION = "# JUCE"


def unescape_cpp(literal):
    """C++ string literal body (UTF-8 source) -> text."""
    data = bytearray()
    i = 0
    raw = literal.encode("utf-8")
    simple = {ord("n"): b"\n", ord("t"): b"\t", ord('"'): b'"', ord("'"): b"'", ord("\\"): b"\\"}
    while i < len(raw):
        c = raw[i]
        if c != ord("\\"):
            data.append(c)
            i += 1
            continue
        nxt = raw[i + 1]
        if nxt == ord("x"):
            j = i + 2
            while j < len(raw) and j < i + 4 and chr(raw[j]) in "0123456789abcdefABCDEF":
                j += 1
            data.append(int(raw[i + 2 : j], 16))
            i = j
        elif nxt in simple:
            data += simple[nxt]
            i += 2
        else:
            raise ValueError("unsupported escape in " + literal)
    return data.decode("utf-8")


def unescape_juce(text):
    """Same as juce::LocalisedStrings: \\" \\' \\t \\r \\n."""
    return text.replace('\\"', '"').replace("\\'", "'").replace("\\t", "\t").replace("\\r", "\r").replace("\\n", "\n")


def find_close_quote(line, start):
    last = ""
    while start < len(line):
        c = line[start]
        if c == '"' and last != "\\":
            return start
        last = c
        start += 1
    return start


def source_texts(root):
    texts = {}
    for path in sorted((root / "src").rglob("*")):
        if path.suffix not in (".h", ".cpp"):
            continue
        content = path.read_text(encoding="utf-8")
        for match in CALL.finditer(content):
            text = "".join(unescape_cpp(part) for part in LITERAL.findall(match.group(1)))
            line = content.count("\n", 0, match.start()) + 1
            texts.setdefault(text, f"{path.relative_to(root)}:{line}")
    return texts


def translations(path):
    """[(text, translation, line number, is JUCE text)] like juce::LocalisedStrings::loadFromText."""
    entries = []
    in_juce = False
    for number, raw in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        line = raw.strip()
        if line == JUCE_SECTION:
            in_juce = True
        if not line.startswith('"'):
            continue
        close = find_close_quote(line, 1)
        text = unescape_juce(line[1:close])
        opening = find_close_quote(line, close + 1)
        close2 = find_close_quote(line, opening + 1)
        entries.append((text, unescape_juce(line[opening + 1 : close2]), number, in_juce))
    return entries


def check(root):
    texts = source_texts(root)
    problems = []
    files = sorted((root / "resources" / "Languages").glob("*.txt"))
    if not files:
        problems.append("no translation files in resources/Languages")
    for path in files:
        name = path.relative_to(root)
        seen = {}
        for text, translation, number, is_juce in translations(path):
            if text in seen:
                problems.append(f"{name}:{number}: translated twice (also line {seen[text]}): {text!r}")
            seen[text] = number
            if not is_juce and text not in texts:
                problems.append(f"{name}:{number}: not a text of the program: {text!r}")
            if not translation:
                problems.append(f"{name}:{number}: empty translation: {text!r}")
            elif sorted(PLACEHOLDER.findall(text)) != sorted(PLACEHOLDER.findall(translation)):
                problems.append(f"{name}:{number}: placeholders differ: {text!r} -> {translation!r}")
        for text, where in texts.items():
            if text not in seen:
                problems.append(f"{name}: missing translation of {text!r} ({where})")
    return problems


def main():
    root = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else pathlib.Path(__file__).resolve().parent.parent)
    problems = check(root)
    for problem in problems:
        print(problem)
    if problems:
        return 1
    print(f"translations complete: {len(source_texts(root))} texts")
    return 0


if __name__ == "__main__":
    sys.exit(main())
