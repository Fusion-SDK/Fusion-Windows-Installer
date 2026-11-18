# Discord account linking

This document defines the account-linking contract shared by Fusion's Discord
bot, Supabase backend, and future in-game account screen.

## User flow

1. A Fusion user runs `/account-verify` in the official Discord server.
2. The bot uses the Discord interaction's immutable user ID to request a
   one-time challenge. It displays the code only in an ephemeral response.
3. The user opens Geometry Dash, signs in to Fusion, and opens
   **Fusion Account → Link Discord** in the in-game menu.
4. The signed-in client submits the code to the Edge Function using the
   current user's Supabase access token.
5. The backend atomically consumes the challenge and creates the one-to-one
   account link.
6. The user runs `/account-status`; the bot reads the authoritative link state
   and synchronizes the configured verified role.

The Discord username is not used as proof of identity and does not need to be
collected during Fusion registration. The backend links the Discord ID from the
original bot interaction to the Fusion user ID authenticated by Supabase.

## API contract

All requests are JSON POST requests to:

```text
https://YOUR_PROJECT.supabase.co/functions/v1/discord-link
```

### Bot: create a challenge

Headers:

- `content-type: application/json`
- `x-fusion-bot-secret: BOT_SECRET`

Body:

```json
{
  "action": "create_challenge",
  "discord_user_id": "Discord snowflake"
}
```

Success response:

```json
{
  "code": "ONE_TIME_CODE",
  "expires_at": "ISO-8601 timestamp"
}
```

### In-game client: confirm a challenge

Headers:

- `content-type: application/json`
- `authorization: Bearer SUPABASE_ACCESS_TOKEN`

Body:

```json
{
  "action": "confirm",
  "code": "ONE_TIME_CODE"
}
```

The client must not send a Discord ID. The backend derives the Fusion user ID
from the validated access token and obtains the Discord ID from the challenge
record. A successful response is `{"linked":true}`.

### Bot: status and unlink

These actions use the bot secret and the immutable Discord user ID:

```json
{"action":"status","discord_user_id":"Discord snowflake"}
```

```json
{"action":"unlink","discord_user_id":"Discord snowflake"}
```

Status returns only whether a link exists. Unlink removes only the mapping for
the caller's Discord ID. The bot synchronizes the configured role after the
backend operation succeeds.

## Security controls

- One-to-one uniqueness for Discord IDs and Fusion auth user IDs.
- Ten-minute, single-use challenge codes generated with a cryptographically
  secure random generator.
- Only SHA-256 hashes of challenge codes are stored.
- Three challenge requests per Discord ID per fifteen minutes.
- Challenge records are inaccessible to anon/authenticated clients through RLS.
- The Edge Function validates the user's Supabase access token before accepting
  a confirmation.
- The bot secret and Supabase service-role key remain server-side.
- The bot never requests Fusion passwords, refresh tokens, or access tokens.
- The Edge Function limits request bodies and rejects non-HTTPS bot endpoints
  through the bot's configuration validation.

## CAPTCHA distinction

The one-time code is an account-ownership challenge, not a human CAPTCHA. It
proves that the person controls the Discord account that initiated the flow
and a signed-in Fusion account. It does not prove that the Discord account is
operated by a human. The backend rate limit and Discord's own abuse controls
are the initial protections; an accessible anti-abuse challenge can be added
later if real abuse warrants it.

## In-game implementation gate

The current native loader does not yet expose a Geometry Dash game API, menu,
or authenticated account UI. The in-game screen must be implemented as part of
the version-specific game adapter and must use the active Fusion session's
access token. Until that work is complete and tested in Geometry Dash,
end-to-end account linking must remain disabled in production.
