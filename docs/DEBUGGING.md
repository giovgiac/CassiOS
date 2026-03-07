# Debugging Methodology

A principled approach to fixing bugs and issues. Follow these four steps in order, and loop until the fix is verified.

## 1. Reproduce

Before investigating, reproduce the issue reliably.

- Write a minimal script or test that demonstrates the problem.
- Capture concrete output: exact values, positions, error messages, raw data.
- Avoid guessing at the cause -- let the reproduction guide diagnosis.

If the issue cannot be reproduced, gather more information before proceeding.

## 2. Diagnose

Examine the reproduction output to identify the root cause.

- Compare actual output against expected output in detail.
- Inspect raw data (e.g., character positions, escape codes, intermediate state) rather than relying on visual inspection alone.
- Question assumptions -- the first hypothesis is often wrong. The menu centering bug appeared to be a layout issue but was actually caused by ANSI escape codes and whitespace collapsing.
- Narrow the scope: isolate which layer (data, logic, rendering) produces the wrong result.

## 3. Fix

Apply the minimal change that addresses the root cause.

- Fix the actual cause, not the symptoms.
- Keep the change focused -- avoid unrelated refactoring.
- Consider whether the fix introduces secondary issues (e.g., changing component structure may break test assertions that depend on output format).

## 4. Verify

Re-run the reproduction and confirm the issue is resolved. **This step is not optional and must not be skipped.**

- Run the exact reproduction from step 1 and confirm the output is now correct.
- Test edge cases and alternate scenarios (e.g., different selections, boundary values, empty inputs).
- Run the full test suite to check for regressions elsewhere.
- **If the issue is still reproducible or regressions are found, loop back to step 2.** Continue iterating until the fix is proven and no regressions exist.

## Summary

```
Reproduce -> Diagnose -> Fix -> Verify
    ^                             |
    |_____________________________|
         (loop until verified)
```

Do not consider a bug fixed until you have proven it through the verify step. A fix that has not been verified is not a fix.
