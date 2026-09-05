load("@rules_java//java:defs.bzl", "java_test")

# Encode/decode round-trip tests, ported from the corpus tests in build.py.
# Each test generates a random sector image, writes it to a flux file, reads it
# back, and checks the result matches, using the EncodeDecodeTest tool.

CORPUS = [
    ("acorndfs", "", "--200"),
    ("agat", "", ""),
    ("amiga", "", ""),
    ("apple2", "", "--140 --drivetype=40"),
    ("atarist", "", "--360"),
    ("atarist", "", "--370"),
    ("atarist", "", "--400"),
    ("atarist", "", "--410"),
    ("atarist", "", "--720"),
    ("atarist", "", "--740"),
    ("atarist", "", "--800"),
    ("atarist", "", "--820"),
    ("bk", "", ""),
    ("brother", "", "--120 --drivetype=40"),
    ("brother", "", "--240"),
    (
        "commodore",
        "scripts/commodore1541_test.textpb",
        "--171 --drivetype=40",
    ),
    (
        "commodore",
        "scripts/commodore1541_test.textpb",
        "--192 --drivetype=40",
    ),
    ("commodore", "", "--800"),
    ("commodore", "", "--1620"),
    ("hplif", "", "--264"),
    ("hplif", "", "--608"),
    ("hplif", "", "--616"),
    ("hplif", "", "--770"),
    ("ibm", "", "--1200"),
    ("ibm", "", "--1232"),
    ("ibm", "", "--1440"),
    ("ibm", "", "--1680"),
    ("ibm", "", "--180 --drivetype=40"),
    ("ibm", "", "--160 --drivetype=40"),
    ("ibm", "", "--320 --drivetype=40"),
    ("ibm", "", "--360 --drivetype=40"),
    ("ibm", "", "--720_96"),
    ("ibm", "", "--720_135"),
    ("mac", "scripts/mac400_test.textpb", "--400"),
    ("mac", "scripts/mac800_test.textpb", "--800"),
    ("n88basic", "", ""),
    ("rx50", "", ""),
    ("tartu", "", "--390 --drivetype=40"),
    ("tartu", "", "--780"),
    ("tids990", "", ""),
    ("victor9k", "", "--612"),
    ("victor9k", "", "--1224"),
]

def _sanitize(s):
    result = ""
    for ch in s.elems():
        result += ch if ch.isalnum() else "_"
    return result

def define_corpus_tests():
    tests = []
    for entry in CORPUS:
        format = entry[0]
        script = entry[1]
        flags = entry[2]
        name = _sanitize(format + script + flags)
        for ext in ["scp", "flux"]:
            test_name = "corpustest_%s_%s" % (name, ext)
            args = [format, ext]
            if flags:
                args += flags.split(" ")
            java_test(
                name = test_name,
                main_class = "com.cowlark.fluxengine.buildtools.EncodeDecodeTest",
                use_testrunner = False,
                args = args,
                runtime_deps = ["//java/com/cowlark/fluxengine/buildtools:encodedecodetest"],
                size = "small",
                timeout = "moderate",
            )
            tests.append(test_name)
    return tests
