# Revise error results

There are many functions that return an unsuccessful `Result` when a
non-recoverable error happens, like running out of heap memory. This polutes
the engine with useless error handling.

## To fix

Only return a `Result` when a recoverable error happens. For the rest, use
`BLK_VERIFY` and early return.

If I want to avoid refactoring existing code, I could keep the older function
signatures marked as deprecated and use the new one from now on.
