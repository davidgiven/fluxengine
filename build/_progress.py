import sys

(_, current, max) = sys.argv
chars = len(str(max))
print(f"[{current:>{chars}}/{max:>{chars}}]")
