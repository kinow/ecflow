//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "VConfigLoader.hpp"

#include <map>

typedef std::map<std::string,VConfigLoader*> Map;

static Map* makers = 0;

VConfigLoader::VConfigLoader(const std::string& name)
{
    if(makers == 0)
        makers = new Map();

    makers->insert(Map::value_type(name,this));
}

VConfigLoader::~VConfigLoader()
{
    // Not called
}

bool VConfigLoader::process(const std::string& name,VProperty *prop)
{
    Map::iterator it = makers->find(name);
    if(it != makers->end())
    {
        (*it).second->load(prop);
        return true;
    }
    return false;
}   