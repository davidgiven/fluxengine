from build.ab import Rule, simplerule
from build.utils import add_wildcard_dependency


@Rule
def git_repository(self, name, url, branch, path, commit=None):
    simplerule(
        replaces=self,
        outs=[f"{path}/.git/config"],
        commands=[
            f"rmdir {path}/.git",
            f"git clone -q {url} --depth=1 -c advice.detachedHead=false -b {branch} {path}",
        ]
        + (
            [
                f"cd {path} && git fetch --depth=1 origin {commit} && git checkout {commit}"
            ]
            if commit
            else []
        ),
        sandbox=False,
        generator=True,
        label="GITREPOSITORY",
    )

    add_wildcard_dependency(self, f"{path}/**/*", exclude="**/.git/**")
