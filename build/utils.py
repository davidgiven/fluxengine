from build.ab import Rule, normalrule, Target, filenameof, Targets
from os.path import basename


@Rule
def objectify(self, name, src: Target, symbol):
    normalrule(
        replaces=self,
        ins=["build/_objectify.py", src],
        outs=[basename(filenameof(src)) + ".h"],
        commands=["$(PYTHON) {ins[0]} {ins[1]} " + symbol + " > {outs}"],
        label="OBJECTIFY",
    )


@Rule
def test(
    self,
    name,
    command: Target = None,
    commands=None,
    ins: Targets = None,
    deps: Targets = None,
    label="TEST",
):
    if command:
        normalrule(
            replaces=self,
            ins=[command],
            outs=["sentinel"],
            commands=["{ins[0]}", "touch {outs}"],
            deps=deps,
            label=label,
        )
    else:
        normalrule(
            replaces=self,
            ins=ins,
            outs=["sentinel"],
            commands=commands + ["touch {outs}"],
            deps=deps,
            label=label,
        )
