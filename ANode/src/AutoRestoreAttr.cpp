//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #9 $
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description :
//============================================================================

#include <sstream>

#include "AutoRestoreAttr.hpp"
#include "Indentor.hpp"
#include "Ecf.hpp"
#include "Node.hpp"
#include "NodeContainer.hpp"
#include "Defs.hpp"
#include "Log.hpp"
#include "Str.hpp"

using namespace std;

namespace ecf {

std::ostream& AutoRestoreAttr::print(std::ostream& os) const
{
   Indentor in;
   Indentor::indent(os) << "autorestore";
   for(size_t i = 0; i < nodes_to_restore_.size(); ++i) os << " " << nodes_to_restore_[i];
   os << "\n";
   return os;
}

std::string AutoRestoreAttr::toString() const
{
   std::stringstream ss;
   ss << "autorestore";
   for(size_t i = 0; i < nodes_to_restore_.size(); ++i) ss << " " << nodes_to_restore_[i];
   return ss.str();
}


bool AutoRestoreAttr::operator==(const AutoRestoreAttr& rhs) const
{
   if (nodes_to_restore_ == rhs.nodes_to_restore_) return true;

#ifdef DEBUG
   if (Ecf::debug_equality()) {
      std::cout << "AutoRestoreAttr::operator== nodes_to_restore_ == rhs.nodes_to_restore_\n";
   }
#endif

   return false;
}

void AutoRestoreAttr::do_autorestore()
{
   string warning_message;
   for(size_t i =0; i <nodes_to_restore_.size(); i++) {

      warning_message.clear();
      node_ptr referenceNode = node_->findReferencedNode( nodes_to_restore_[i] , warning_message);
      if (!referenceNode.get()) {
         /// Could not find the references node
         std::stringstream ss;
         ss << "AutoRestoreAttr::do_auto_restore: " << node_->debugType() << " references a path '" << nodes_to_restore_[i]  << "' which can not be found\n";
         log(Log::ERR,ss.str());
         continue;
      }

      NodeContainer* nc = referenceNode->isNodeContainer();
      if (nc) {
         try { nc->restore();}
         catch( std::exception& e) {
            std::stringstream ss; ss << "AutoRestoreAttr::do_auto_restore: could not autorestore : because : " << e.what();
            log(Log::ERR,ss.str());
         }
      }
      else {
         std::stringstream ss;
         ss << "AutoRestoreAttr::do_auto_restore: " << node_->debugType() << " references a node '" << nodes_to_restore_[i]  << "' which can not be restored. Only family and suite nodes can be restored";
         log(Log::ERR,ss.str());
      }
   }
}

void AutoRestoreAttr::check(std::string& errorMsg) const
{
   string warning_message;
   for(size_t i =0; i < nodes_to_restore_.size(); i++) {

      warning_message.clear();
      node_ptr referenceNode = node_->findReferencedNode( nodes_to_restore_[i] , warning_message);
      if (!referenceNode.get()) {
         /// Could not find the references node

         // OK a little bit of duplication, since findReferencedNode, will also look for externs
         // See if the Path:name is defined as an extern, in which case *DONT* error:
         // This is client side specific, since server does not have externs.
         if (node_->defs()->find_extern( nodes_to_restore_[i], Str::EMPTY())) {
            continue;
         }

         std::stringstream ss;
         ss << "Error: autorestore on node " << node_->debugType() << " references a path '" << nodes_to_restore_[i]  << "' which can not be found\n";
         errorMsg += ss.str();
         continue;
      }

      // reference node found, make sure it not a task
      NodeContainer* nc = referenceNode->isNodeContainer();
      if (!nc) {
          std::stringstream ss;
          ss << "Error: autorestore on node " << node_->debugType() << " references a node '" << nodes_to_restore_[i]  << "' which is a task. restore only works with suites or family nodes";
          errorMsg += ss.str();
      }
   }
}

}