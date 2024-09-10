from build.ab import (
    Rule,
    simplerule,
    TargetsMap,
    filenameof,
    emit,
)

emit(
    """
ZIP ?= zip
ZIPNOTE ?= zipnote
"""
)


@Rule
def zip(
    self, name, flags="", items: TargetsMap = {}, extension="zip", label="ZIP"
):
    cs = ["rm -f {outs[0]}"]

    ins = []
    for k, v in items.items():
        cs += [
            "cat %s | $(ZIP) -q %s {outs[0]} -" % (filenameof(v), flags),
            "printf '@ -\\n@=%s\\n' | $(ZIPNOTE) -w {outs[0]}" % k,
        ]
        ins += [v]

    simplerule(
        replaces=self,
        ins=ins,
        outs=[f"={self.localname}." + extension],
        commands=cs,
        label=label,
    )
