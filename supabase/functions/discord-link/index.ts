import { createClient } from "npm:@supabase/supabase-js@2";

const corsHeaders = {
  "access-control-allow-origin": "*",
  "access-control-allow-headers": "authorization, content-type, x-fusion-bot-secret",
  "access-control-allow-methods": "POST, OPTIONS",
};

const json = (body: Record<string, unknown>, status = 200) =>
  new Response(JSON.stringify(body), {
    status,
    headers: { ...corsHeaders, "content-type": "application/json" },
  });

const encoder = new TextEncoder();
const alphabet = "23456789ABCDEFGHJKLMNPQRSTUVWXYZ";

function constantTimeEqual(a: string, b: string): boolean {
  const left = encoder.encode(a);
  const right = encoder.encode(b);
  let diff = left.length ^ right.length;
  const length = Math.max(left.length, right.length);
  for (let i = 0; i < length; i++) diff |= (left[i] ?? 0) ^ (right[i] ?? 0);
  return diff === 0;
}

function validDiscordId(value: unknown): value is string {
  return typeof value === "string" && /^[0-9]{17,20}$/.test(value);
}

function makeCode(): string {
  const bytes = new Uint8Array(10);
  crypto.getRandomValues(bytes);
  return Array.from(bytes, (byte) => alphabet[byte % alphabet.length]).join("");
}

async function sha256(value: string): Promise<string> {
  const digest = await crypto.subtle.digest("SHA-256", encoder.encode(value));
  return Array.from(new Uint8Array(digest), (byte) => byte.toString(16).padStart(2, "0")).join("");
}

async function cleanupExpiredChallenges(admin: ReturnType<typeof createClient>): Promise<void> {
  const cleanupBefore = new Date(Date.now() - 24 * 60 * 60 * 1000).toISOString();
  const { error } = await admin.from("discord_link_challenges")
    .delete()
    .lt("created_at", cleanupBefore);
  if (error) console.error("Discord challenge cleanup failed:", error.message);
}

Deno.serve(async (request) => {
  if (request.method === "OPTIONS") return new Response("ok", { headers: corsHeaders });
  if (request.method !== "POST") return json({ error: "method_not_allowed" }, 405);

  const supabaseUrl = Deno.env.get("SUPABASE_URL");
  const anonKey = Deno.env.get("SUPABASE_ANON_KEY");
  const serviceKey = Deno.env.get("SUPABASE_SERVICE_ROLE_KEY");
  const botSecret = Deno.env.get("FUSION_BOT_API_SECRET");
  if (!supabaseUrl || !anonKey || !serviceKey || !botSecret) {
    return json({ error: "service_not_configured" }, 503);
  }

  const rawBody = await request.text();
  if (encoder.encode(rawBody).byteLength > 4096) return json({ error: "request_too_large" }, 413);

  let body: Record<string, unknown>;
  try {
    const parsed: unknown = JSON.parse(rawBody);
    if (!parsed || typeof parsed !== "object" || Array.isArray(parsed)) {
      return json({ error: "invalid_json" }, 400);
    }
    body = parsed as Record<string, unknown>;
  } catch {
    return json({ error: "invalid_json" }, 400);
  }

  const action = body.action;
  const admin = createClient(supabaseUrl, serviceKey, { auth: { persistSession: false } });


  if (action === "confirm") {
    const authorization = request.headers.get("authorization") ?? "";
    const match = authorization.match(/^Bearer\s+(.+)$/i);
    if (!match) return json({ error: "authentication_required" }, 401);
    const userClient = createClient(supabaseUrl, anonKey, { auth: { persistSession: false } });
    const { data: userData, error: userError } = await userClient.auth.getUser(match[1]);
    if (userError || !userData.user) return json({ error: "invalid_session" }, 401);
    await cleanupExpiredChallenges(admin);

    const code = typeof body.code === "string" ? body.code.trim().toUpperCase() : "";
    if (!/^[23456789ABCDEFGHJKLMNPQRSTUVWXYZ]{10}$/.test(code)) return json({ error: "invalid_code" }, 400);
    const { data, error } = await admin.rpc("confirm_discord_link", {
      p_fusion_user_id: userData.user.id,
      p_code_hash: await sha256(code),
    });
    if (error) return json({ error: "verification_unavailable" }, 503);
    if (data !== true) return json({ error: "invalid_or_expired_code" }, 400);
    return json({ linked: true });
  }

  const suppliedSecret = request.headers.get("x-fusion-bot-secret") ?? "";
  if (!constantTimeEqual(suppliedSecret, botSecret)) return json({ error: "unauthorized" }, 401);
  await cleanupExpiredChallenges(admin);

  const discordUserId = body.discord_user_id;
  if (!validDiscordId(discordUserId)) return json({ error: "invalid_discord_user_id" }, 400);

  if (action === "create_challenge") {
    const code = makeCode();
    const expiresAt = new Date(Date.now() + 10 * 60 * 1000).toISOString();
    const { data, error } = await admin.rpc("create_discord_link_challenge", {
      p_discord_user_id: discordUserId,
      p_code_hash: await sha256(code),
      p_expires_at: expiresAt,
    });
    if (error) return json({ error: "verification_unavailable" }, 503);
    if (data !== true) return json({ error: "rate_limited" }, 429);
    return json({ code, expires_at: expiresAt });
  }

  if (action === "status") {
    const { data, error } = await admin
      .from("discord_account_links")
      .select("discord_user_id")
      .eq("discord_user_id", discordUserId)
      .maybeSingle();
    if (error) return json({ error: "verification_unavailable" }, 503);
    return json({ linked: Boolean(data) });
  }

  if (action === "unlink") {
    const { error } = await admin
      .from("discord_account_links")
      .delete()
      .eq("discord_user_id", discordUserId);
    if (error) return json({ error: "verification_unavailable" }, 503);
    return json({ unlinked: true });
  }

  return json({ error: "unknown_action" }, 400);
});
