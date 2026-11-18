-- Fusion catalog release and community foundation.
-- This migration stores release metadata only. It does not expose artifact
-- storage or mint download URLs; that requires a trusted backend that verifies
-- signed format 3 packages before publishing them.

create table if not exists public.mod_releases (
    id uuid primary key default gen_random_uuid(),
    mod_id uuid not null references public.mods(id) on delete cascade,
    version text not null check (char_length(version) between 1 and 64),
    channel text not null default 'stable'
        check (channel in ('stable', 'beta')),
    package_format_version smallint not null
        check (package_format_version = 3),
    artifact_bucket text not null default 'fusion-mods'
        check (char_length(artifact_bucket) between 1 and 128),
    artifact_path text not null
        check (
            char_length(artifact_path) between 1 and 512
            and artifact_path !~ '(^/|(^|/)\.\.(/|$))'
        ),
    signature_bucket text not null default 'fusion-mods'
        check (char_length(signature_bucket) between 1 and 128),
    signature_path text not null
        check (
            char_length(signature_path) between 1 and 512
            and signature_path !~ '(^/|(^|/)\.\.(/|$))'
        ),
    manifest_sha256 text not null
        check (manifest_sha256 ~ '^[0-9a-f]{64}$'),
    status text not null default 'pending'
        check (status in ('pending', 'published', 'yanked', 'rejected')),
    created_by uuid references auth.users(id) on delete set null,
    created_at timestamptz not null default now(),
    published_at timestamptz,
    unique (mod_id, version, channel)
);

create index if not exists mod_releases_published_lookup
    on public.mod_releases (mod_id, channel, published_at desc)
    where status = 'published';

create table if not exists public.mod_reviews (
    id uuid primary key default gen_random_uuid(),
    mod_id uuid not null references public.mods(id) on delete cascade,
    user_id uuid not null references auth.users(id) on delete cascade,
    rating smallint not null check (rating between 1 and 5),
    body text not null default ''
        check (char_length(body) <= 2000),
    is_hidden boolean not null default false,
    created_at timestamptz not null default now(),
    updated_at timestamptz not null default now(),
    unique (mod_id, user_id)
);

create index if not exists mod_reviews_visible_lookup
    on public.mod_reviews (mod_id, created_at desc)
    where is_hidden = false;

create table if not exists public.mod_download_events (
    id uuid primary key default gen_random_uuid(),
    release_id uuid not null references public.mod_releases(id) on delete cascade,
    user_id uuid not null references auth.users(id) on delete cascade,
    event_type text not null
        check (event_type in ('started', 'completed', 'failed')),
    created_at timestamptz not null default now()
);

create index if not exists mod_download_events_user_lookup
    on public.mod_download_events (user_id, created_at desc);

alter table public.mod_releases enable row level security;
alter table public.mod_reviews enable row level security;
alter table public.mod_download_events enable row level security;

drop policy if exists "Users can read published releases for approved mods"
    on public.mod_releases;
create policy "Users can read published releases for approved mods"
on public.mod_releases for select to authenticated
using (
    status = 'published'
    and exists (
        select 1 from public.mods m
        where m.id = mod_releases.mod_id
          and m.status = 'approved'
    )
);

drop policy if exists "Users can read visible reviews for approved mods"
    on public.mod_reviews;
create policy "Users can read visible reviews for approved mods"
on public.mod_reviews for select to authenticated
using (
    is_hidden = false
    and exists (
        select 1 from public.mods m
        where m.id = mod_reviews.mod_id
          and m.status = 'approved'
    )
);

drop policy if exists "Users can review approved mods"
    on public.mod_reviews;
create policy "Users can review approved mods"
on public.mod_reviews for insert to authenticated
with check (
    auth.uid() = user_id
    and is_hidden = false
    and exists (
        select 1 from public.mods m
        where m.id = mod_reviews.mod_id
          and m.status = 'approved'
    )
);

drop policy if exists "Users can update own visible reviews"
    on public.mod_reviews;
create policy "Users can update own visible reviews"
on public.mod_reviews for update to authenticated
using (auth.uid() = user_id and is_hidden = false)
with check (auth.uid() = user_id and is_hidden = false);

drop policy if exists "Users can read own download events"
    on public.mod_download_events;
create policy "Users can read own download events"
on public.mod_download_events for select to authenticated
using (auth.uid() = user_id);

drop policy if exists "Users can record own downloads of published releases"
    on public.mod_download_events;
create policy "Users can record own downloads of published releases"
on public.mod_download_events for insert to authenticated
with check (
    auth.uid() = user_id
    and exists (
        select 1
        from public.mod_releases r
        join public.mods m on m.id = r.mod_id
        where r.id = mod_download_events.release_id
          and r.status = 'published'
          and m.status = 'approved'
    )
);

-- Do not grant clients write access to release metadata or moderation state.
-- A trusted server-side publisher/moderation workflow must use the service role.
grant select on public.mod_releases to authenticated;
grant select, insert, select, update on public.mod_reviews to authenticated;
grant select, insert on public.mod_download_events to authenticated;

-- Column-level grants prevent users from changing moderation fields or authorship.
revoke insert, update on public.mod_reviews from authenticated;
grant insert (mod_id, user_id, rating, body)
    on public.mod_reviews to authenticated;
grant update (rating, body)
    on public.mod_reviews to authenticated;
