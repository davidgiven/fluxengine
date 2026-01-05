from build.pkg import package
from build.c import clibrary
from build.git import git_repository

package(
    name="md4c_lib",
    package="md4c",
    fallback=clibrary(
        name="md4c_fallback_lib",
        srcs=[
            "dep/md4c/src/entity.c",
            "dep/md4c/src/entity.h",
            "dep/md4c/src/md4c-html.h",
            "dep/md4c/src/md4c.c",
            "dep/md4c/src/md4c.h",
        ],
        hdrs={"md4c.h": "dep/md4c/src/md4c.h"},
        deps=[
            git_repository(
                name="md4c_repo",
                url="https://github.com/mity/md4c",
                branch="master",
                path="dep/md4c",
            )
        ],
    ),
)
