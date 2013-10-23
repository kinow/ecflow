//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #40 $ 
//
// Copyright 2009-2012 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
//============================================================================

#include <sstream>

#include "TimeAttr.hpp"
#include "Indentor.hpp"
#include "Calendar.hpp"
#include "PrintStyle.hpp"
#include "Ecf.hpp"

namespace ecf {

void TimeAttr::calendarChanged( const ecf::Calendar& c )
{
   if ( makeFree_ ) {
      return;
   }

   if (timeSeries_.calendarChanged(c)) {
      state_change_no_ = Ecf::incr_state_change_no();
   }

   // For a time series, we rely on the re queue to reset makeFree
   if (isFree(c)) {
      setFree();
   }
}

void TimeAttr::resetRelativeDuration()
{
   if (timeSeries_.resetRelativeDuration()) {
      state_change_no_ = Ecf::incr_state_change_no();
   }
}

std::ostream& TimeAttr::print(std::ostream& os) const
{
   Indentor in;
   Indentor::indent(os) << toString();
   if (!PrintStyle::defsStyle()) {
      os << timeSeries_.state_to_string(makeFree_);
   }
   os << "\n";
   return os;
}

std::string TimeAttr::toString() const
{
   std::string ret = "time ";
   ret += timeSeries_.toString();
   return ret;
}

std::string TimeAttr::dump() const
{
	std::stringstream ss;
	ss << "time ";

    if (makeFree_) ss << "(free) ";
    else           ss << "(holding) ";

 	ss << timeSeries_.toString();

 	return ss.str();
}

bool TimeAttr::operator==(const TimeAttr& rhs) const
{
	if (makeFree_ != rhs.makeFree_) {
		return false;
	}
 	return timeSeries_.operator==(rhs.timeSeries_);
}

bool TimeAttr::structureEquals(const TimeAttr& rhs) const
{
   return timeSeries_.structureEquals(rhs.timeSeries_);
}

bool TimeAttr::isFree(const ecf::Calendar& calendar) const
{
	// The FreeDepCmd can be used to free the time,
 	if (makeFree_) {
		return true;
	}
 	return is_free(calendar);
}

bool TimeAttr::is_free(const ecf::Calendar& calendar) const
{
	return timeSeries_.isFree(calendar);
}

void TimeAttr::setFree() {
	makeFree_ = true;
	state_change_no_ = Ecf::incr_state_change_no();

#ifdef DEBUG_STATE_CHANGE_NO
	std::cout << "TimeAttr::setFree()\n";
#endif
}

void TimeAttr::clearFree() {
	makeFree_ = false;
	state_change_no_ = Ecf::incr_state_change_no();

#ifdef DEBUG_STATE_CHANGE_NO
	std::cout << "TimeAttr::clearFree()\n";
#endif
}

void TimeAttr::miss_next_time_slot()
{
   timeSeries_.miss_next_time_slot();
   state_change_no_ = Ecf::incr_state_change_no();
}


bool TimeAttr::why(const ecf::Calendar& c, std::string& theReasonWhy) const
{
	if (isFree(c)) return false;
	theReasonWhy += " is time dependent";

	// This can apply to single and series
	boost::posix_time::time_duration calendar_time = timeSeries_.duration(c);
	if (calendar_time < timeSeries_.start().duration()) {
	    timeSeries_.why(c, theReasonWhy);
	    return true;
	}

	if (timeSeries_.hasIncrement()) {
	   if (calendar_time > timeSeries_.start().duration() && calendar_time < timeSeries_.finish().duration()) {
	       timeSeries_.why(c, theReasonWhy);
	       return true;
	   }
	}

	// For a single slot, we are always free after single slot time, hence we must look for tomorrow.
	// If we are in series then, we past the last time slot
   // Find the next date that matches
   boost::gregorian::date_duration one_day(1);
   boost::gregorian::date the_next_date = c.date();  // todays date
   the_next_date += one_day;                         // add one day, so its in the future

   theReasonWhy += " ( next run is at ";
   theReasonWhy += timeSeries_.start().toString();
   theReasonWhy += " ";
   theReasonWhy += to_simple_string( the_next_date );
   theReasonWhy += " )";
 	return true;
}
}
