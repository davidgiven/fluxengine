from build.ab import Rule, normalrule, Target, filenameof
from os.path import basename


@Rule
def objectify(self, name, src: Target, symbol):
    normalrule(
        replaces=self,
        ins=[src],
        outs=[basename(filenameof(src)) + ".h"],
        commands=["xxd -i -n " + symbol + " {ins} > {outs}"],
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
