from build.ab import Rule, normalrule, Target, filenameof
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
def test(self, name, command: Target):
    normalrule(
        replaces=self,
        ins=[command],
        outs=["sentinel"],
        commands=["{ins[0]}", "touch {outs}"],
        label="TEST",
    )
