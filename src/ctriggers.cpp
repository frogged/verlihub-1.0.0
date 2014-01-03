/*
	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
	Copyright (C) 2006-2014 Verlihub Project, devs at verlihub-project dot org

	Verlihub is free software; You can redistribute it
	and modify it under the terms of the GNU General
	Public License as published by the Free Software
	Foundation, either version 3 of the license, or at
	your option any later version.

	Verlihub is distributed in the hope that it will be
	useful, but without any warranty, without even the
	implied warranty of merchantability or fitness for
	a particular purpose. See the GNU General Public
	License for more details.

	Please see http://www.gnu.org/licenses/ for a copy
	of the GNU General Public License.
*/

#include "ctriggers.h"
#include "cconfigitembase.h"
#include "cserverdc.h"
#include <time.h>
#include "i18n.h"
#include "stringutils.h"

namespace nVerliHub {
	using namespace nUtils;
	using namespace nEnums;
	using namespace nSocket;
	using namespace nMySQL;
	namespace nTables {
  /**
  Class constructor.
  @param[in] server A pointer to cServerDC object
  */

cTriggers::cTriggers( cServerDC *server ) :
	tMySQLMemoryList<cTrigger, cServerDC>(server->mMySQL, server, "file_trigger")
{
	SetClassName("nDC::cTriggers");
}

  /**

  Create columns for file_trigger table. The columns are:
  - command (CHAR - primary key - length 15) : the command that is used to show a trigger message
  - send_as (CHAR - length 15) : the optional sender name
  - def (TEXT) : the text of the trigger if it is not contained in a file
  - descr (TEXT) : the trigger's description
  - min_class (INT - length 2) : the min class to trigger
  - max_class (INT - length 2) : the max class to trigger
  - flags (INT - length 2) : the mask in order to speficy trigger's options
  - seconds (INT - length 15) :  timeout for trigger. 0 means the timer is not actived
  */

void cTriggers::AddFields()
{
	AddCol("command", "varchar(15)", "", false, mModel.mCommand);
	AddPrimaryKey("command");
	AddCol("send_as", "varchar(25)", "hub-security", true, mModel.mSendAs);
	AddCol("def", "text", "", true, mModel.mDefinition);
	AddCol("descr", "text", "", true, mModel.mDescription);
	AddCol("min_class", "int(2)", "", true, mModel.mMinClass);
	AddCol("max_class", "int(2)", "10", true, mModel.mMaxClass);
	AddCol("flags", "int(2)", "0", true, mModel.mFlags);
	AddCol("seconds", "int(15)", "0", true, mModel.mSeconds);
	mMySQLTable.mExtra = "PRIMARY KEY(command)";
	SetBaseTo(&mModel);
}

  /**

  Check if timeout for a trigger is expired. If so run it

  @param[in] now The current Unix Time in seconds
  */

void cTriggers::OnTimer(long now)
{
	 istringstream is;
	 iterator it;
	 cTrigger *trigger;
	 for (it= begin(); it != end(); ++it)  {
		trigger = *it;
		if(!trigger->mSeconds) continue;
		long next = trigger->mLastTrigger + trigger->mSeconds;
		if (next < now) {
			trigger->mLastTrigger = now;
			trigger->DoIt(is, NULL, *mOwner, true);
		}
	 }
}

  /**

  Trigger all existing trigger by a given mask. There are 2 cases when this method is called:
  1) When a user logs in
  2) When +help is triggered

  @param[in] FlagMask The mask
  @param[in] conn A pointer to a user's connection
  */

void cTriggers::TriggerAll(int FlagMask, cConnDC *conn)
{
	istringstream is;
	iterator it;
	cTrigger *trigger;
	for (it= begin(); it != end(); ++it) {
		trigger = *it;
		if (trigger->mFlags & FlagMask)
			trigger->DoIt(is, conn, *mOwner);
	}
}

  /**

  Compare 2 cTrigger objects to determine if they have the same key. This method is used to forbid duplicated entry when when a new trigger is added

  @param[in,out] D1 The frist trigger
  @param[in,out] D2 The second trigger
  */

bool cTriggers::CompareDataKey(const cTrigger &D1, const cTrigger &D2)
{
	return D1.mCommand == D2.mCommand;
}

  /**

  Run a trigger by a given command

  @param[in,out] conn The user's connection
  @param[in,out] cmd The sent command by the user
  @param[in,out] cmd_line The stream or command line
  @param[in,out] server Reference to cServerDC
  @return False on failure or true on success
  */

bool cTriggers::DoCommand(cConnDC *conn, const string &cmd, istringstream &cmd_line, cServerDC &server)
{
	cTrigger * curTrigger;
	for(int i = 0; i < Size(); ++i) {
		curTrigger = (*this)[i];
		if(curTrigger->mMinClass <= conn->mpUser->mClass && cmd == curTrigger->mCommand) {
			if(Log(3))
				LogStream() << "trigger found " << cmd << endl;
			return curTrigger->DoIt (cmd_line, conn, server);
		}
	}
	return false;
}

  /**

  Class constructor

  */

cTriggerConsole::cTriggerConsole(cDCConsole *console) : tTriggerConsoleBase(console)
{
	this->AddCommands();
}

  /**

  Class destructor

  */

cTriggerConsole::~cTriggerConsole()
{}

  /**

  Show help to the user if he is wrong when he types a command

  @param[in] cmd The type of the command (list, add, mod or del)
  @param[in,out] os The stream where to store the output

  */

void cTriggerConsole::GetHelpForCommand(int cmd, ostream &os)
{
	string help_str;
	switch(cmd)
	{
		case eLC_LST:
		help_str = "!lsttrigger\r\nGive a list of triggers";
		break;
		case eLC_ADD:
		case eLC_MOD:
		help_str = "!(add|mod)trigger <trigger>"
			"[ -d <\"definition\">]"
			"[ -h <help_desc>]"
			"[ -f <flags>]"
			"[ -n <sendas_nick>]"
			"[ -c <min_class>]"
			"[ -C <max_class>]";
		break;
		case eLC_DEL:
		help_str = "!deltrigger <trigger>"; break;
		default: break;
	}
	cDCProto::EscapeChars(help_str,help_str);
	os << help_str;
}

  /**

  Show a complete help when user type (+|!)htrigger

  @param[in,out] os The stream where to store the output

  */

void cTriggerConsole::GetHelp(ostream &os)
{
	string help;

	help = "http://verlihub-project.org/doc/page/manual.messages#File_Triggers\r\n\r\n";

	help += " Available trigger flags:\r\n\r\n";
	help += " 0\t\t\t- Send to main chat, visible to user only\r\n";
	help += " 1\t\t\t- Execute command\r\n";
	help += " 2\t\t\t- Message is sent to PM\r\n";
	help += " 4\t\t\t- Automatically trigger when user logs in\r\n";
	help += " 8\t\t\t- Trigger on help command\r\n";
	help += " 16\t\t\t- The definition is the text\r\n";
	help += " 32\t\t\t- Allow replacing of variables\r\n";
	help += " 64\t\t\t- Message is sent to everyone in main chat\r\n\r\n";
	help += " Remember to make the sum of selected above flags.\r\n\r\n";

	help += " Available variables:\r\n\r\n";
	help += " %[PARALL]\t\t- Trigger parameters\r\n";
	help += " %[PAR1]\t\t- First trigger parameter, separated by space\r\n";
	help += " %[END1]\t\t- Last trigger parameter, separated by space\r\n";
	help += " %[CC]\t\t\t- User country code\r\n";
	help += " %[CN]\t\t\t- User country name\r\n";
	help += " %[IP]\t\t\t- User IP\r\n";
	help += " %[HOST]\t\t- User host\r\n";
	help += " %[NICK]\t\t\t- User nick\r\n";
	help += " %[CLASS]\t\t- User class number\r\n";
	help += " %[CLASSNAME]\t\t- User class name\r\n";
	help += " %[SHARE]\t\t- User share size\r\n";
	help += " %[SHARE_EXACT]\t\t- User exact share size\r\n";
	help += " %[USERS]\t\t- Online users\r\n";
	help += " %[USERS_ACTIVE]\t- Online active users\r\n";
	help += " %[USERS_PASSIVE]\t- Online passive users\r\n";
	help += " %[USERSPEAK]\t\t- Peak online users\r\n";
	help += " %[UPTIME]\t\t- Hub uptime\r\n";
	help += " %[VERSION]\t\t- Hub version\r\n";
	help += " %[HUBNAME]\t\t- Hub name\r\n";
	help += " %[HUBTOPIC]\t\t- Hub topic\r\n";
	help += " %[HUBDESC]\t\t- Hub description\r\n";
	help += " %[VERSION_DATE]\t- Hub version date\r\n";
	help += " %[TOTAL_SHARE]\t\t- Total share\r\n";
	help += " %[SHAREPEAK]\t\t- Peak total share\r\n";
	help += " %[ss]\t\t\t- Current second, 2 digits\r\n";
	help += " %[mm]\t\t\t- Current minute, 2 digits\r\n";
	help += " %[HH]\t\t\t- Current hour, 2 digits\r\n";
	help += " %[DD]\t\t\t- Current day, 2 digits\r\n";
	help += " %[MM]\t\t\t- Current month, 2 digits\r\n";
	help += " %[YY]\t\t\t- Current year, 4 digits\r\n";

	cDCProto::EscapeChars(help, help);
	os << help;
}

  /**

  Return the regex for a given command

  @param[in] cmd The type of the command (list, add, mod or del)

  */

const char * cTriggerConsole::GetParamsRegex(int cmd)
{
	switch(cmd)
	{
		case eLC_ADD:
		case eLC_MOD:
			return "^(\\S+)("
			"( -d ?(\")?((?(4)[^\"]+?|\\S+))(?(4)\"))?|" //[ -d<definition>|"<<definition>>]"
			"( -h ?(\")?((?(7)[^\"]+?|\\S+))(?(7)\"))?|" //[ -h(<help>|"<<help>>")]
			"( -f ?(-?\\d+))?|" //[ -f<flags>]
			"( -n ?(\\S+))?|" // [ -n<sendas_nick>]
			"( -c ?(-?\\d+))?|" //[ -c<min_class>]
			"( -C ?(-?\\d+))?|" //[ -c<max_class>]
			"( -t ?(\\S+))?|" //[ -t<timeout>]
			")*\\s*$"; // the end of message
		case eLC_DEL:
			return "(\\S+)";
		default: return "";break;
	};
}


bool cTriggerConsole::CheckData(cfBase *cmd, cTrigger &data)
{
	if(data.mDefinition.empty()) {
		*cmd->mOS << _("The definition is empty or not specified. Please define it with -d option.");
		return false;
	}
	size_t pos = data.mDefinition.rfind("dbconfig");
	if(pos != string::npos) {
		*cmd->mOS << _("It's not allowed to define dbconfig file as trigger.") << "\n";
		cConnDC *conn = (cConnDC *) cmd->mConn;
		ostringstream message;
		message << autosprintf(_("User '%s' tried to define dbconfig as trigger"), conn->mpUser->mNick.c_str());
		mOwner->mServer->ReportUserToOpchat(conn, message.str());
		return false;
	}
	FilterPath(data.mDefinition);
	string vPath(mOwner->mServer->mConfigBaseDir), triggerPath, triggerName;
	ExpandPath(vPath);
	GetPath(data.mDefinition, triggerPath, triggerName);
	ReplaceVarInString(triggerPath, "CFG", triggerPath, vPath);
	ExpandPath(triggerPath);
	if((triggerPath.substr(0,vPath.length()) != vPath)) {
		*cmd->mOS << autosprintf(_("The file %s for the trigger %s must be locate in VerliHub config folder (use %%[CFG] variable; for ex %%[CFG]/%s)"), data.mDefinition.c_str(), data.mCommand.c_str(), triggerName.c_str());
		return false;
	}
	return true;
}

  /**

  Read, extract data from a trigger command and save the trigger

  @param[in] cmd cfBase object
  @param[in] CmdID Not used here
  @param[in,out] data The trigger to add or modify

  */
bool cTriggerConsole::ReadDataFromCmd(cfBase *cmd, int CmdID, cTrigger &data)
{
	cTrigger tmp = data;
	enum {eADD_ALL, eADD_CMD, eADD_CHOICE,
		eADD_DEFp, eADD_QUOTE, eADD_DEF,
		eADD_DESCp, eADD_QUOTE2, eADD_DESC,
		eADD_FLAGSp, eADD_FLAGS,
		eADD_NICKp, eADD_NICK,
		eADD_CLASSp, eADD_CLASS,
		eADD_CLASSXp, eADD_CLASSX,
		eADD_TIMEOUTp, eADD_TIMEOUT
		};
	cmd->GetParStr(eADD_CMD,tmp.mCommand);
	cmd->GetParUnEscapeStr(eADD_DEF,tmp.mDefinition);
	cmd->GetParStr(eADD_DESC,tmp.mDescription);
	cmd->GetParStr(eADD_NICK,tmp.mSendAs);
	cmd->GetParInt(eADD_FLAGS, tmp.mFlags);
	cmd->GetParInt(eADD_CLASS, tmp.mMinClass);
	cmd->GetParInt(eADD_CLASSX, tmp.mMaxClass);
	string sTimeout("0");
	cmd->GetParStr(eADD_TIMEOUT,sTimeout);
	tmp.mSeconds = mOwner->mServer->Str2Period(sTimeout,*cmd->mOS);

	bool checkDefinition = !(tmp.mFlags & eTF_DB);
	if(CmdID == eLC_ADD && checkDefinition && !CheckData(cmd,tmp)) {
		return false;
	} else if(CmdID == eLC_MOD && !data.mCommand.empty() && checkDefinition && !CheckData(cmd,tmp)) {
		return false;
	}
	data = tmp;
	return true;
}

  /**

  Return a pointer to cTriggers object

  @return cTriggers pointer

  */

cTriggers *cTriggerConsole::GetTheList()
{
	return mOwner->mTriggers;
}

  /**

  Return the command suffix

  @return The string of suffix

  */

const char *cTriggerConsole::CmdSuffix(){ return "trigger";}

  /**

  Return the command prefix

  @todo Use config variable for prefix  (cmd_start_user)
  @return The string of prefix

  */

  const char *cTriggerConsole::CmdPrefix(){ return "!"; }

  /**

  The header of the message to show when !lsttrigger is sent

  @param[in] os The stream where to store the message

  */

void cTriggerConsole::ListHead(ostream *os)
{
	(*os) << "\r\n ";
	(*os) << setw(30) << setiosflags(ios::left) << toUpper(_("Name"));
	(*os) << setw(25) << setiosflags(ios::left) << toUpper(_("Definition"));
	(*os) << setw(8) << setiosflags(ios::left) << toUpper(_("Flag"));
	(*os) << setw(15) << setiosflags(ios::left) << toUpper(_("Send as"));
	(*os) << setw(10) << setiosflags(ios::left) << toUpper(_("Min class"));
	(*os) << toUpper(_("Max class")) << "\r\n";
	(*os) << " " << string(30+25+8+10+10+15,'=');
}

  /**

  Return true if a given connection has rights to show, add, edit and remove triggers

  @todo Add mod_trigger_class in config
  @param[in] conn The connection
  @param[in] cmd Not used here

  */

bool cTriggerConsole::IsConnAllowed(cConnDC *conn,int cmd)
{
	return (conn && conn->mpUser && conn->mpUser->mClass >= eUC_ADMIN);
}

};
};