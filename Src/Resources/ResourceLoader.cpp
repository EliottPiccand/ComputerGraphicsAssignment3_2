#include "Resources/ResourceLoader.h"


bool ResourceLoader::isLoaded(std::string_view name)
{
    return resources_.contains(std::string(name));
}
