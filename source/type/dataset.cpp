/* Copyright © 2001-2014, Canal TP and/or its affiliates. All rights reserved.

This file is part of Navitia,
    the software to build cool stuff with public transport.

Hope you'll enjoy and contribute to this project,
    powered by Canal TP (www.canaltp.fr).
Help us simplify mobility and open public transport:
    a non ending quest to the responsive locomotion way of traveling!

LICENCE: This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.

Stay tuned using
twitter @navitia
IRC #navitia on freenode
https://groups.google.com/d/forum/navitia
www.navitia.io
*/

#include "type/dataset.h"
#include "type/indexes.h"
#include "type/pt_data.h"
#include "type/serialization.h"
#include <boost/date_time/gregorian/greg_serialize.hpp>
#include <boost/date_time/posix_time/time_serialize.hpp>

namespace navitia {
namespace type {

template <class Archive>
void Dataset::serialize(Archive& ar, const unsigned int) {
    ar& idx& uri& contributor& realtime_level& validation_period& desc& system;
}
SERIALIZABLE(Dataset)

Indexes Dataset::get(Type_e type, const PT_Data&) const {
    Indexes result;
    switch (type) {
        case Type_e::Contributor:
            result.insert(contributor->idx);
            break;
        case Type_e::VehicleJourney:
            return indexes(vehiclejourney_list);
        default:
            break;
    }
    return result;
}

}  // namespace type
}  // namespace navitia
