-- Fusion Circle starter schema.
-- Run in a dedicated Supabase project before enabling online catalog features.

create extension if not exists pgcrypto;

create table if not exists public.profiles (
    id uuid primary key references auth.users(id) on delete cascade,
    display_name text not null check (char_length(display_name) between 2 and 32),
    created_at timestamptz not null default now()
);

create table if not exists public.mods (
    id uuid primary key default gen_random_uuid(),
    name text not null,
    description text not null default '',
    author text,
    latest_version text,
    status text not null default 'pending'
        check (status in ('pending', 'approved', 'rejected', 'suspended')),
    created_by uuid references auth.users(id) on delete set null,
    created_at timestamptz not null default now()
);

create table if not exists public.mod_submissions (
    id uuid primary key default gen_random_uuid(),
    user_id uuid not null references auth.users(id) on delete cascade,
    name text not null,
    description text not null,
    source_url text,
    status text not null default 'pending'
        check (status in ('pending', 'approved', 'rejected')),
    created_at timestamptz not null default now()
);

create table if not exists public.user_sync (
    user_id uuid primary key references auth.users(id) on delete cascade,
    preferences jsonb not null default '{}'::jsonb,
    favorites jsonb not null default '[]'::jsonb,
    updated_at timestamptz not null default now()
);

alter table public.profiles enable row level security;
alter table public.mods enable row level security;
alter table public.mod_submissions enable row level security;
alter table public.user_sync enable row level security;

create policy "Users can read own profile"
on public.profiles for select to authenticated
using (auth.uid() = id);

create policy "Users can create own profile"
on public.profiles for insert to authenticated
with check (auth.uid() = id);

create policy "Users can update own profile"
on public.profiles for update to authenticated
using (auth.uid() = id)
with check (auth.uid() = id);

create policy "Authenticated users can read approved mods"
on public.mods for select to authenticated
using (status = 'approved');

create policy "Users can read own submissions"
on public.mod_submissions for select to authenticated
using (auth.uid() = user_id);

create policy "Users can submit mods"
on public.mod_submissions for insert to authenticated
with check (auth.uid() = user_id);

create policy "Users can manage own sync data"
on public.user_sync for all to authenticated
using (auth.uid() = user_id)
with check (auth.uid() = user_id);

-- Profile creation trigger.
create or replace function public.handle_new_user()
returns trigger
language plpgsql
security definer
set search_path = ''
as $$
begin
    insert into public.profiles (id, display_name)
    values (
        new.id,
        coalesce(
            nullif(new.raw_user_meta_data ->> 'display_name', ''),
            split_part(new.email, '@', 1)
        )
    );
    return new;
end;
$$;

drop trigger if exists on_auth_user_created on auth.users;
create trigger on_auth_user_created
after insert on auth.users
for each row execute procedure public.handle_new_user();
