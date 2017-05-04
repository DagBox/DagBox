/*
  Copyright 2017 Kaan Genç

  This file is part of DagBox.

  DagBox is free software: you can redistribute it and/or modify it
  under the terms of the GNU Lesser General Public License as
  published by the Free Software Foundation, either version 3 of the
  License, or (at your option) any later version.

  DagBox is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with DagBox.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include <msgpack.hpp>


namespace logic
{
    /*! \brief A simple relation between keys.
     */
    struct fact
    {
        std::string name;
        std::vector<std::string> keys;

        MSGPACK_DEFINE_MAP(name, keys);
    };

    /*! \brief Relations between keys that need to be satisfied.
     */
    struct condition
    {
        /*! Relations that need to be satisfied.
         */
        std::string name;
        /*! A condition may contain any number of variables. Each
         * variable is represented using a number, which corresponds
         * to a variable in the rule containing this condition. The
         * positions of these variables in the vector correspond with
         * their positions in the facts, or the left-hand sides of the
         * rules.
         */
        std::vector<uint> variable;

        MSGPACK_DEFINE_MAP(name, variable);
    };

    /*! \brief A relation between keys that is true if some conditions are true.
     *
     * A rule is composed of two parts, a left-hand side and some
     * conditions. The left-hand side proposes a relation between some
     * variables. The conditions represent the relations that are
     * adequate to prove that the left-hand side relation is true.
     *
     * Note that the conditions are adequate to prove the relation,
     * but not always necessary. There may be multiple rules with the
     * same name but different conditions, and any of these rules can
     * also prove the relation.
     */
    struct rule
    {
        /*! Name of the rule. */
        std::string name;
        /*! Number of variables on the left-hand side of the rule. */
        uint lfs;
        /*! Conditions that are adequate to prove the relation on the
         *  left-hand side of the rule.
         */
        std::vector<condition> conditions;

        MSGPACK_DEFINE_MAP(name, lfs, conditions);
    };
};
