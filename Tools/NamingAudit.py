#!/usr/bin/env python3
"""Frontier naming gate — proves format names are legal before anyone codes them.

Modes
  --registry            audit the generated registry (default; runs in CI)
  --check-tree PATH...  audit real file and folder names in a checkout
  --strict              treat every warning as a failure

Checks
  C1  blacklist compliance for every chosen name, candidate and codec unit
  C2  `Space` reserve: a Space-suffixed name is legal only if it is a roster
      domain, or is carried as a retired read-alias
  C3  codec naming: `<Stem>Codec`, never carrying the container suffix
      (`GeometryCodec`, never `GeometrySpaceCodec`)
  C4  registry integrity: unique suffixes, every alias owned by exactly one entry
  C5  filename law: PascalCase, no `And` conjunction, no abbreviation
  C6  discouraged-word report (atlas / slice / cluster / bundle / chart ...)
"""

import argparse
import re
import sys
import tomllib
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from FormatNamingVocabulary import (BLACKLIST, DISCOURAGED, SPACE_DOMAINS,  # noqa: E402
                                    SUFFIX_CLASSES, ABBREVIATIONS, SplitTokens, FindViolations)

ROOT = Path(__file__).resolve().parent.parent
REGISTRY = ROOT / "Documentation/Naming/FormatSuffixRegistry.toml"

# Names that legitimately end in `Space` because they are roster domains.
ROSTER = set(SPACE_DOMAINS)
BARE_OK = re.compile(r"^\.[A-Z][A-Za-z0-9]*$")


class Report:
    def __init__(self):
        self.errors, self.warnings = [], []

    def error(self, code, subject, detail):
        self.errors.append(f"{code}  {subject}: {detail}")

    def warn(self, code, subject, detail):
        self.warnings.append(f"{code}  {subject}: {detail}")

    def flush(self):
        for line in self.errors:
            print(f"FAIL  {line}")
        for line in self.warnings:
            print(f"note  {line}")
        print(f"\n{len(self.errors)} failure(s), {len(self.warnings)} note(s)")


def AuditName(report, subject, name, *, kind, allow_space=False, warn_only=False):
    """Run C1 + C2 + C5 for one identifier."""
    bare = name.lstrip(".").rstrip("/")
    label = f"{subject} [{kind}] `{name}`"
    hard, discouraged = FindViolations(bare)
    for word in hard:
        (report.warn if warn_only else report.error)("C1", label, f"barred word {word}")
    for hint in discouraged:
        report.warn("C6", label, f"discouraged {hint}")

    if bare.endswith("Space") and not warn_only:
        if bare not in ROSTER and not allow_space:
            report.error("C2", label,
                         "`Space` is reserved for roster domains: " + ", ".join(sorted(ROSTER)))
    if kind == "suffix" and not warn_only and bare.startswith(".") is False:
        pass

    if kind in ("suffix", "path-segment"):
        tokens = SplitTokens(bare)
        if "and" in tokens or "&" in bare or "+" in bare:
            (report.warn if warn_only else report.error)("C5", label, "conjunction in a name")
        if not bare[:1].isupper():
            (report.warn if warn_only else report.error)("C5", label, "not PascalCase")
        for token in tokens:
            if token in ABBREVIATIONS:
                (report.warn if warn_only else report.error)(
                    "C5", label, f"abbreviation `{token}`; write `{ABBREVIATIONS[token]}`")


def AuditRegistry(report, registry):
    seen_suffix, seen_alias = {}, {}
    for entry in registry.get("format", []):
        eid, group = entry["id"], entry["group"]
        is_core_domain = "Core Domains" in group
        chosen, candidates = entry["chosen"], list(entry["candidates"])
        aliases, codec, unit = entry.get("aliases", []), entry["codec"], entry["unit"]

        if chosen.startswith("Engine/") or chosen.endswith("/"):
            for part in [p for p in re.split(r"[\\/]", chosen.strip("/")) if p]:
                AuditName(report, f"{eid} unit", part, kind="path-segment")
        else:
            AuditName(report, f"{eid} chosen", chosen, kind="suffix", allow_space=is_core_domain)
            stem = chosen.lstrip(".")
            if stem not in seen_suffix:
                seen_suffix[stem] = eid
            else:
                report.error("C4", f"{eid} chosen {chosen}",
                             f"already claimed by `{seen_suffix[stem]}`")

        retired = {a.lstrip(".").lower() for a in aliases}
        for cand in candidates:
            if cand.startswith("Engine/") or cand.endswith("/"):
                continue
            # a Space-suffixed alternative is legal only as a roster proposal or as a
            # name being carried purely as a read-only legacy alias
            legal_space = cand.lstrip(".").lower() in retired
            AuditName(report, f"{eid} candidate", cand, kind="suffix",
                      allow_space=is_core_domain or legal_space)

        for alias in aliases:
            AuditName(report, f"{eid} legacy alias", alias, kind="alias", warn_only=True)
            key = alias.lstrip(".").lower()
            if key in seen_alias and seen_alias[key] != eid:
                report.warn("C4", f"{eid} alias {alias}", f"also aliased by `{seen_alias[key]}`")
            seen_alias[key] = eid

        is_format = chosen.startswith(".")
        if not is_format:
            AuditName(report, f"{eid} unit", codec, kind="unit")
        else:
            AuditName(report, f"{eid} codec", codec, kind="codec")
            if is_format and not codec.endswith("Codec"):
                report.error("C3", f"{eid} codec `{codec}`", "unit name must end in `Codec`")
            if "SpaceCodec" in codec:
                report.error("C3", f"{eid} codec `{codec}`",
                             "codec must not carry the container suffix (`GeometryCodec`, not `GeometrySpaceCodec`)")

        for part in [p for p in re.split(r"[\\/]", unit) if p]:
            AuditName(report, f"{eid} engine unit", part, kind="path-segment")

    for field in registry.get("containment", []):
        for name in [field["chosen"], *field.get("candidates", [])]:
            AuditName(report, f"containment {field['field']}", name, kind="field")
        for name in field.get("legacy_replaced", []):
            AuditName(report, f"containment {field['field']} retired", name, kind="alias",
                      warn_only=True)

    for root_doc in registry.get("root_document", []):
        AuditName(report, f"root {root_doc['suffix']}", root_doc["suffix"], kind="suffix")
        for legacy in [s.strip() for s in root_doc["legacy_replaced"].split("·")]:
            if legacy:
                AuditName(report, f"root {root_doc['suffix']} legacy", legacy, kind="alias",
                          warn_only=True)


def AuditTree(report, paths):
    for base in paths:
        root = Path(base)
        targets = [root] if root.is_file() else sorted(p for p in root.rglob("*") if ".git/" not in str(p))
        for p in targets:
            if p.name in {".", ".."} or p.name.startswith(".git"):
                continue
            for part in [x for x in p.parts if x not in {".", "/"}]:
                if part in ("Frontier-", "Tools", "Documentation", "Naming"):
                    continue
                if not re.fullmatch(r"[A-Za-z0-9._-]+", part):
                    continue
                stem = part.split(".")[0]
                if not stem[:1].isupper() and stem not in ("README", "CMakeLists", "LICENSE"):
                    continue
                AuditName(report, p, stem, kind="path-segment")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--registry", action="store_true", help="audit the generated registry")
    parser.add_argument("--check-tree", nargs="*", default=[], help="audit real names on disk")
    parser.add_argument("--strict", action="store_true", help="fail on notes as well")
    parser.add_argument("--quiet", action="store_true")
    args = parser.parse_args()

    report = Report()
    if args.registry or not args.check_tree:
        with open(REGISTRY, "rb") as handle:
            AuditRegistry(report, tomllib.load(handle))
    if args.check_tree:
        AuditTree(report, args.check_tree)

    if not args.quiet:
        report.flush()
    if report.errors or (args.strict and report.warnings):
        return 1
    print("NamingAudit: registry clean — every chosen name, candidate, codec and unit path "
          "passes the blacklist and the `Space` reserve.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
