-- Discord-to-Fusion account linking.
-- Pending codes are never readable by anon/authenticated clients. Only trusted
-- Edge Functions using the service role can create, inspect, or consume them.

create table if not exists public.discord_link_challenges (
    id uuid primary key default gen_random_uuid(),
    discord_user_id text not null check (discord_user_id ~ '^[0-9]{17,20}$'),
    code_hash text not null unique check (code_hash ~ '^[a-f0-9]{64}$'),
    created_at timestamptz not null default now(),
    expires_at timestamptz not null,
    consumed_at timestamptz,
    check (expires_at > created_at)
);

create index if not exists discord_link_challenges_user_created_idx
    on public.discord_link_challenges (discord_user_id, created_at desc);

create table if not exists public.discord_account_links (
    discord_user_id text primary key check (discord_user_id ~ '^[0-9]{17,20}$'),
    fusion_user_id uuid not null unique references auth.users(id) on delete cascade,
    linked_at timestamptz not null default now()
);

alter table public.discord_link_challenges enable row level security;
alter table public.discord_account_links enable row level security;

revoke all on public.discord_link_challenges from anon, authenticated;
revoke all on public.discord_account_links from anon, authenticated;
grant all on public.discord_link_challenges to service_role;
grant all on public.discord_account_links to service_role;

create or replace function public.create_discord_link_challenge(
    p_discord_user_id text,
    p_code_hash text,
    p_expires_at timestamptz
)
returns boolean
language plpgsql
security definer
set search_path = ''
as $$
declare
    recent_count integer;
begin
    if p_discord_user_id !~ '^[0-9]{17,20}$'
       or p_code_hash !~ '^[a-f0-9]{64}$'
       or p_expires_at <= now()
       or p_expires_at > now() + interval '15 minutes' then
        return false;
    end if;

    perform pg_advisory_xact_lock(hashtextextended(p_discord_user_id, 0));

    select count(*) into recent_count
    from public.discord_link_challenges
    where discord_user_id = p_discord_user_id
      and created_at > now() - interval '15 minutes';

    if recent_count >= 3 then
        return false;
    end if;

    update public.discord_link_challenges
       set consumed_at = now()
     where discord_user_id = p_discord_user_id
       and consumed_at is null;

    insert into public.discord_link_challenges (discord_user_id, code_hash, expires_at)
    values (p_discord_user_id, p_code_hash, p_expires_at);

    return true;
end;
$$;

create or replace function public.confirm_discord_link(
    p_fusion_user_id uuid,
    p_code_hash text
)
returns text
language plpgsql
security definer
set search_path = ''
as $$
declare
    challenge public.discord_link_challenges%rowtype;
    inserted_count integer;
begin
    select * into challenge
    from public.discord_link_challenges
    where code_hash = p_code_hash
      and consumed_at is null
      and expires_at > now()
    for update;

    if not found then
        return 'invalid_or_expired';
    end if;

    insert into public.discord_account_links (discord_user_id, fusion_user_id)
    values (challenge.discord_user_id, p_fusion_user_id)
    on conflict do nothing;

    get diagnostics inserted_count = row_count;

    update public.discord_link_challenges
       set consumed_at = now()
     where id = challenge.id;

    if inserted_count = 0 then
        return 'account_already_linked';
    end if;

    return 'linked';
end;
$$;

revoke all on function public.create_discord_link_challenge(text, text, timestamptz) from public, anon, authenticated;
revoke all on function public.confirm_discord_link(uuid, text) from public, anon, authenticated;
grant execute on function public.create_discord_link_challenge(text, text, timestamptz) to service_role;
grant execute on function public.confirm_discord_link(uuid, text) to service_role;
