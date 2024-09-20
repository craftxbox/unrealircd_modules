/*
 *   Unreal Internet Relay Chat Daemon, src/modules/third/antikill.c
 *   (C) 2023 craftxbox
 *
 *   Permission is hereby granted, free of charge, to any person obtaining a copy
 *   of this software and associated documentation files (the "Software"), to deal
 *   in the Software without restriction, including without limitation the rights
 *   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *   copies of the Software, and to permit persons to whom the Software is
 *   furnished to do so, subject to the following conditions:
 *   
 *   The above copyright notice and this permission notice shall be included in all
 *   copies or substantial portions of the Software.
 *   
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *   SOFTWARE.
 */

#include "unrealircd.h"

static char buf[BUFSIZE], buf2[BUFSIZE];

ModuleHeader MOD_HEADER = {
    "third/antikill",
    "1.0",
    "HOOKTYPE_PRE_REMOTE_TO_LOCAL_KILL",
    "craftxbox",
    "unrealircd-6",
};

int antikill_check(Client *client, Client *target, const char *reason);

MOD_LOAD() { return MOD_SUCCESS; }

MOD_UNLOAD() { return MOD_SUCCESS; }

MOD_INIT()
{
  HookAdd(modinfo->handle, HOOKTYPE_PRE_REMOTE_TO_LOCAL_KILL, -10, antikill_check);
  return MOD_SUCCESS;
}

int antikill_check(Client *client, Client *target, const char *reason)
{
  if (!MyConnect(target))
  {
    return EX_ALLOW;
  }

  if (!ValidatePermissionsForPath("immune:kill", target, NULL, NULL, NULL))
  {
    return EX_ALLOW;
  }

  sendnumeric(client, ERR_KILLDENY, target->name);

  sendto_prefix_one(target, client, NULL, ":%s KILL %s : %s", client->name,
                    target->name, reason);

  unreal_log(
      ULOG_INFO, "kill", "KILL_COMMAND", client,
      "Rejected KILL: $target.details [by: $client] ($reason) (User is Immune)",
      log_data_client("target", target), log_data_string("reason", reason));

  MessageTag *mtags = NULL;
  new_message(client, NULL, &mtags);

  if (!MyConnect(client))
  {
    // Propagate the kill, Else we desync the network and are about to introduce
    // a user that already exists.
    sendto_server(client, 0, 0, mtags, ":%s KILL %s :%s", client->id,
                  target->id, reason);

    build_umode_string(target, 0, SEND_UMODES | UMODE_SERVNOTICE, buf);

    sendto_serv_butone_nickcmd(target->direction, mtags, target,
                               (*buf == '\0' ? "+" : buf));

    broadcast_moddata_client(target);

    char modebuf[BUFSIZE];

    Membership *channels;
    for (channels = target->user->channel; channels;
         channels = channels->next)
    {
      Channel *channel = channels->channel;
      char *modes;
      int chanops_only = invisible_user_in_channel(target, channel);

      /* If the user is banned, don't send any rejoins, it would only be
       * annoying */
      if (is_banned(target, channel, BANCHK_JOIN, NULL, NULL))
        continue;

      sendto_server(target, 0, 0, mtags, ":%s SJOIN %lld %s :%s", me.id,
                    (long long)channel->creationtime, channel->name,
                    target->id);

      modes = get_chmodes_for_user(client, channels->member_modes);
      if (!BadPtr(modes))
        sendto_server(target, 0, 0, mtags, ":%s MODE %s %s 0", me.name, channel->name, modes);
    }
  }

  return EX_DENY;
}
