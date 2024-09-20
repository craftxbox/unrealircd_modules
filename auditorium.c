/* Copyright (C) All Rights Reserved
** Written by Gottem <support@gottem.nl>
** Modifed by craftxbox <spam@crxb.cc>
** Website: https://gottem.nl/unreal
** License: https://gottem.nl/unreal/license
*/

/*** <<<MODULE MANAGER START>>>
module {
	documentation "https://github.com/craftxbox/unrealircd_modules/auditorium.md";
	troubleshooting "In case of problems, PM me @craftxbox on libera.chat";
	min-unrealircd-version "6.*";
	post-install-text {
		"The module is installed, now all you need to do is add a 'loadmodule' line to your config file:";
		"loadmodule \"third/auditorium\";";
		"Then /rehash the IRCd.";
		"For usage information, refer to the module's documentation found at: https://github.com/craftxbox/unrealircd_modules/auditorium.md";
	}
}
*** <<<MODULE MANAGER END>>>
*/

// One include for all cross-platform compatibility thangs
#include "unrealircd.h"

#define CheckAPIError(apistr, apiobj) \
	do { \
		if(!(apiobj)) { \
			config_error("A critical error occurred on %s for %s: %s", (apistr), MOD_HEADER.name, ModuleGetErrorStr(modinfo->handle)); \
			return MOD_FAILED; \
		} \
	} while(0)

// Quality fowod declarations
int auditorium_chmode_isok(Client *client, Channel *channel, char mode, const char *para, int checkt, int what);
int auditorium_hook_cansend_chan(Client *client, Channel *channel, Membership *lp, const char **text, const char **errmsg, SendType sendtype);

#if (UNREAL_VERSION_MAJOR < 1 || (UNREAL_VERSION_MAJOR == 1 && UNREAL_VERSION_MINOR == 0))
	int auditorium_hook_visibleinchan(Client *target, Channel *channel);
#else
	int auditorium_hook_visibleinchan(Client *target, Channel *channel, Member *client_member);
#endif

#define CHMODE_FLAG 'u' // Good ol' +u ;];]
#define IsAudit(x) ((x) && has_channel_mode((x), CHMODE_FLAG))

// Muh globals
Cmode_t extcmode_auditorium = 0L; // Store bitwise value latur

// Dat dere module header
ModuleHeader MOD_HEADER = {
	"third/auditorium", // Module name
	"3.0.0", // Version
	"Channel mode +u to show channel events/messages to/from people with +o/+a/+q only", // Description
	"Gottem, craftxbox", // Author
	"unrealircd-6", // Modversion
};

typedef struct {
	char* speakerModes;
} auditoriumParam;

void *modeu_put_param(void *data, const char *para) {
	auditoriumParam *r = (auditoriumParam *)data;

	if(!r) {
		r = safe_alloc(sizeof(auditoriumParam));
	}

	r->speakerModes = strdup(para);

	return (void *) r;
}

const char *modeu_get_param(void *data)
{
	auditoriumParam *r = (auditoriumParam *)data;
	static char buf[32];

	if (!r)
		return NULL;
	ircsnprintf(buf, sizeof(buf), "%s", r->speakerModes);
	return buf;
}

int modeu_free_param(void *para, int _soft)
{
	safe_free(para);
	return 0;
}

void *modeu_dup_struct(void *src)
{
	auditoriumParam *dst = safe_alloc(sizeof(auditoriumParam));

	memcpy(dst, src, sizeof(auditoriumParam));
	return (void *)dst;
}

int modeu_sjoin_check(Channel *channel, void *our, void *their)
{
	auditoriumParam *ourp = (auditoriumParam *)our;
	auditoriumParam *theirp = (auditoriumParam *)their;


	char *ours = ourp->speakerModes;
	char *theirs = theirp->speakerModes;

	char *temp = safe_alloc(6);

	int i = -1;

	if(strchr(ours,'v') || strchr(theirs,'v')) {
		temp[++i] = 'v';
	}

	if(strchr(ours,'h') || strchr(theirs,'h')) {
		temp[++i] = 'h';
	}

	if(strchr(ours,'o') || strchr(theirs,'o')) {
		temp[++i] = 'o';
	}

	if(strchr(ours,'a') || strchr(theirs,'a')) {
		temp[++i] = 'a';
	}

	if(strchr(ours,'q') || strchr(theirs,'q')) {
		temp[++i] = 'q';
	}

	char *temp2 = safe_alloc(i+1);

	memcpy(temp2, temp, i+1);

	ourp->speakerModes = strdup(temp2);

	free(temp);
	free(temp2);

	return EXSJ_MERGE;
}

const char * modeu_conv_param(const char *param, Client *_client, Channel *_channel) {
	return param;
}

// Initialisation routine (register hooks, commands and modes or create structs etc)
MOD_INIT() {
	// Request the mode flag
	CmodeInfo cmodereq;
	memset(&cmodereq, 0, sizeof(cmodereq));
	cmodereq.letter = CHMODE_FLAG; // Flag yo
	cmodereq.paracount = 1;
	cmodereq.is_ok = auditorium_chmode_isok; // Custom verification function
	cmodereq.get_param = modeu_get_param;
	cmodereq.put_param = modeu_put_param;
	cmodereq.dup_struct = modeu_dup_struct;
	cmodereq.free_param = modeu_free_param;
	cmodereq.sjoin_check = modeu_sjoin_check;
	cmodereq.conv_param = modeu_conv_param;
	CheckAPIError("CmodeAdd(extcmode_auditorium)", CmodeAdd(modinfo->handle, cmodereq, &extcmode_auditorium));

	MARK_AS_GLOBAL_MODULE(modinfo);

	HookAdd(modinfo->handle, HOOKTYPE_VISIBLE_IN_CHANNEL, 0, auditorium_hook_visibleinchan);
	HookAdd(modinfo->handle, HOOKTYPE_CAN_SEND_TO_CHANNEL, 999, auditorium_hook_cansend_chan); // Low prio hook to make sure we go after everything else (like anticaps etc)
	return MOD_SUCCESS;
}

// Actually load the module here (also command overrides as they may not exist in MOD_INIT yet)
MOD_LOAD() {
	return MOD_SUCCESS; // We good
}

// Called on unload/rehash obv
MOD_UNLOAD() {
	return MOD_SUCCESS; // We good
}

int auditorium_chmode_isok(Client *client, Channel *channel, char mode, const char *para, int checkt, int what) {
	/* Args:
	** client: Client who issues the MODE change
	** channel: Channel to which the MODE change applies
	** mode: The mode character for completeness
	** para: Parameter to the channel mode (will be NULL for paramless modes)
	** checkt: Check type, one of EXCHK_*. Explained later.
	** what: Used to differentiate between adding and removing the mode, one of MODE_ADD or MODE_DEL
	*/

	/* Access types:
	** EXCHK_ACCESS: Verify if the user may (un)set the mode, do NOT send error messages for this (just check access)
	** EXCHK_ACCESS_ERR: Similar to above, but you SHOULD send an error message here
	** EXCHK_PARAM: Check the sanity of the parameter(s)
	*/

	/* Return values:
	** EX_ALLOW: Allow it
	** EX_DENY: Deny for most people (only IRC opers w/ override may use it)
	** EX_ALWAYS_DENY: Even prevent IRC opers from overriding shit
	*/
	if((checkt == EXCHK_ACCESS) || (checkt == EXCHK_ACCESS_ERR)) { // Access check lol
		// Check if the user has +a or +q (OperOverride automajikally overrides this bit ;])
		if(!check_channel_access(client, channel, "aqY")) {
			if(checkt == EXCHK_ACCESS_ERR)
				sendnumeric(client, ERR_CHANOWNPRIVNEEDED, channel->name);
			return EX_DENY;
		}
		return EX_ALLOW;
	}
	else if (checkt == EXCHK_PARAM)
		{
		for(int i = 0; i < strlen(para); i++) {
			int s = para[i];
			if(s == 'v') continue;
			if(s == 'h') continue;
			if(s == 'o') continue;
			if(s == 'a') continue;
			if(s == 'q') continue;
			sendnotice(client, "ChanMode +u: ERROR: Parameter must be one or more of vhoaq");
			return EX_DENY;
		}
		return EX_ALLOW;
	}
	return EX_ALLOW; // Fallthrough, like when someone attempts +u 10 it'll simply do +u
}

#if (UNREAL_VERSION_MAJOR < 1 || (UNREAL_VERSION_MAJOR == 1 && UNREAL_VERSION_MINOR == 0))
	int auditorium_hook_visibleinchan(Client *target, Channel *channel)
#else
	int auditorium_hook_visibleinchan(Client *target, Channel *channel, Member *client_member)
#endif
{
	auditoriumParam *param = GETPARASTRUCT(channel, CHMODE_FLAG);

	// due to internal reasons, unreal refuses to hide users that have +vhoaq
	// and also refuses to hide anyone from users with +hoaq
	if(IsAudit(channel) && !check_channel_access(target, channel, param->speakerModes) 
	#if __has_include("ojoin.c")
		&& !check_channel_access(target, channel, "Y") 
	#else
		&& !IsOper(target) // show opers anyway if we dont have third/ojoin
	#endif
		&& !IsULine(target))
		return -1; // ...don't show in /names etc
	return HOOK_CONTINUE;
}

int auditorium_hook_cansend_chan(Client *client, Channel *channel, Membership *lp, const char **text, const char **errmsg, SendType sendtype) {
	auditoriumParam *param = GETPARASTRUCT(channel, CHMODE_FLAG);

	// Let's not act on TAGMSG for the time being :>
	if(sendtype != SEND_TYPE_PRIVMSG && sendtype != SEND_TYPE_NOTICE)
		return HOOK_CONTINUE;
	if(!text || !*text) // If there's no text then the message is already blocked :>
		return HOOK_CONTINUE;

	int notice = (sendtype == SEND_TYPE_NOTICE);
	MessageTag *mtags = NULL;

	if(IsAudit(channel) && IsUser(client) && 
		!check_channel_access(client, channel, param->speakerModes) &&  // regular check if user has the modes specified in +u
#if __has_include("ojoin.c")
		!check_channel_access(client, channel, "Y") && // Use OJOIN to bypass +u if we have the third/ojoin module
#else
		!IsOper(client) && // don't hide messages originating from IRCops
#endif
		!IsULine(client)) { // check if user is an IRCop (OJOINed) or U:lined
		// In case the user is banned just keep processing the hooks as usual, since one of them will finally interrupt and (prolly) emit a message =]
		if(is_banned(client, channel, BANCHK_MSG, text, NULL))
			return HOOK_CONTINUE;

		// ..."relay" the message to +o etc only
		new_message(client, NULL, &mtags);

		char* speakerModes = param->speakerModes;
#ifdef __has_include("ojoin.c") // add Y to speakerModes if third/ojoin is installed, don't hide messages away from users with +Y
		char* tempSpeakerModes = safe_alloc(strlen(speakerModes) + 1);
		strcpy(tempSpeakerModes, speakerModes);
		strcat(tempSpeakerModes, "Y");
		speakerModes = tempSpeakerModes;
#endif // no else, if an oper needs to bypass +u they will have to SAMODE themselves accordingly.

		sendto_channel(channel, client, NULL, speakerModes, 0, SEND_ALL, mtags, ":%s %s @%s :%s", client->name, (notice ? "NOTICE" : "PRIVMSG"), channel->name, *text);
		*text = NULL;
		free_message_tags(mtags);
		// Can't return HOOK_DENY here cuz Unreal might abort() in that case :D
	}
	return HOOK_CONTINUE;
}
