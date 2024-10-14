import sys

try:
    (_, current, max) = sys.argv
    percent = int(100 * float(current) / float(max))
    print(f"[{percent:>3}%]")
except:
    pass
