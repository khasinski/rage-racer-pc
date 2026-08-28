#!/usr/bin/env python3
"""The car price table is the head of the tune-up table in retail layout.

The game reads one contiguous run of prices: a car costs table[i] and its
tune-up costs table[i+1]. Keeping separate, independently aligned arrays
would insert padding and give every car after the first a wrong tune-up cost.
"""

from __future__ import annotations

import re
import struct
import sys
from pathlib import Path


def table(source: str, name: str) -> list[int]:
    match = re.search(rf"unsigned char {name}\[(\d+)\][^=]*=\s*\{{([^}}]*)\}};", source)
    if match is None:
        raise AssertionError(f"{name} is not defined as a byte table")
    values = [int(x, 0) for x in match.group(2).split(",") if x.strip()]
    values += [0] * (int(match.group(1)) - len(values))
    return list(struct.unpack(f"<{len(values) // 4}i", bytes(values[: len(values) // 4 * 4])))


def main() -> int:
    source = (Path(sys.argv[1]) / "src/port/host_state.c").read_text()
    prices = table(source, "g_CarPriceTable")
    tuneUp = table(source, "g_CarTuneUpPriceTable")

    if len(prices) < len(tuneUp) + 1:
        raise AssertionError(
            f"the price table holds {len(prices)} entries, too few to cover "
            f"the {len(tuneUp)} cars the tune-up table prices")
    if prices[0] != 2600:
        raise AssertionError(f"the first car costs {prices[0]}, expected 2600")
    for index, value in enumerate(tuneUp):
        if prices[index + 1] != value:
            raise AssertionError(
                f"price[{index + 1}] is {prices[index + 1]} but tune-up[{index}] "
                f"is {value}; the two tables have drifted apart")
    if not any(value > 1_000_000 for value in prices):
        raise AssertionError("no expensive car in the table; it looks truncated")

    print(f"car prices run contiguously with the tune-up table across {len(prices)} entries")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
