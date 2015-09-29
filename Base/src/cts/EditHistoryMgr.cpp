/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #72 $
//
// Copyright 2009-2012 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
#include "EditHistoryMgr.hpp"
#include "ClientToServerCmd.hpp"
#include "AbstractServer.hpp"
#include "Defs.hpp"
#include "Log.hpp"
#include "Node.hpp"
#include "SuiteChanged.hpp"
#include "Ecf.hpp"
#include "Str.hpp"

using namespace std;
using namespace boost;
using namespace ecf;

EditHistoryMgr::EditHistoryMgr(const ClientToServerCmd* c,AbstractServer* a)
: cts_cmd_(c),
  as_(a),
  state_change_no_(Ecf::state_change_no()),
  modify_change_no_(Ecf::modify_change_no())
{
   assert(cts_cmd_->edit_history_nodes_.empty());
}

EditHistoryMgr::~EditHistoryMgr()
{
   // check if state changed
   if (state_change_no_ != Ecf::state_change_no() || modify_change_no_ != Ecf::modify_change_no()) {

      // Ignore child commands for edit history, where only interested in user commands
      if (!cts_cmd_->task_cmd() && as_->defs()) {

         // *ONLY* record edit history to commands that change the data model
         // Otherwise we will end up making a data model change for read only commands
         // If there has been a change in defs state then the command must return true from isWrite
         if (cts_cmd_->isWrite()) {
            if (cts_cmd_->edit_history_nodes_.empty()) {

               as_->defs()->flag().set(Flag::MESSAGE);
               add_edit_history(Str::ROOT_PATH());
            }
            else {
               size_t the_size = cts_cmd_->edit_history_nodes_.size();
               for(size_t i = 0; i < the_size; i++) {
                  node_ptr edited_node = cts_cmd_->edit_history_nodes_[i].lock();
                  if (edited_node.get()) {
                     // Setting the flag will make a state change. But its OK command allows it.
                     SuiteChanged0 suiteChanged(edited_node);
                     edited_node->flag().set(Flag::MESSAGE);  // trap state change in suite for sync
                     add_edit_history(edited_node->absNodePath());
                  }
               }
            }
         }
         else {
            // Read only command, that is making data model changes, oops ?
            // TODO, Can happen when check pt command set late flag, even though ist read only command.
            //       i.e when saving takes more the 30 seconds
            std::stringstream ss;
            cts_cmd_->print(ss);
            cout << "cmd " << ss.str() << " should return true from isWrite() ******************\n";
            cout << "Read only command is making data changes to defs ?????\n";
            cout << "Ecf::state_change_no() " << Ecf::state_change_no() << " Ecf::modify_change_no() " << Ecf::modify_change_no() << "\n";
            cout << "state_change_no_       " << state_change_no_       << " modify_change_no_       " << modify_change_no_ << "\n";
            cout.flush();
         }
      }
   }

   // Clear edit history nodes;
   cts_cmd_->edit_history_nodes_.clear();
}

void EditHistoryMgr::add_edit_history(const std::string& path) const
{
   // record all the user edits to the node. Reuse the time stamp cache created in handleRequest()
   std::stringstream ss;
   ss << "MSG:";
   if (Log::instance()) ss << Log::instance()->get_cached_time_stamp();
   cts_cmd_->print(ss);
   as_->defs()->add_edit_history(path,ss.str());
}
