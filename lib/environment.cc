#include "lib/globals.h"
#include "lib/environment.h"

static std::unique_ptr<std::set<LocalBase*>> variables;

void Environment::reset()
{
    for (LocalBase* var : *variables)
        var->reset();
}

void Environment::addVariable(LocalBase* local)
{
    if (!variables)
        variables = std::make_unique<std::set<LocalBase*>>();
    variables->insert(local);
}
