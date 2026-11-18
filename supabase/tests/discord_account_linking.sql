begin;

do $$
declare
    fusion_user_one uuid := gen_random_uuid();
    fusion_user_two uuid := gen_random_uuid();
    discord_one text := '12345678901234567';
    discord_two text := '22345678901234567';
    discord_three text := '32345678901234567';
    result text;
begin
    insert into auth.users (id) values (fusion_user_one), (fusion_user_two);

    if not public.create_discord_link_challenge(
        discord_one, repeat('a', 64), now() + interval '10 minutes'
    ) then
        raise exception 'first challenge creation should succeed';
    end if;

    result := public.confirm_discord_link(fusion_user_one, repeat('a', 64));
    if result <> 'linked' then
        raise exception 'first confirmation should link, got %', result;
    end if;

    if not public.create_discord_link_challenge(
        discord_one, repeat('b', 64), now() + interval '10 minutes'
    ) then
        raise exception 'second challenge creation should succeed';
    end if;

    result := public.confirm_discord_link(fusion_user_two, repeat('b', 64));
    if result <> 'account_already_linked' then
        raise exception 'duplicate Discord link should be rejected, got %', result;
    end if;

    if not public.create_discord_link_challenge(
        discord_two, repeat('c', 64), now() + interval '10 minutes'
    ) then
        raise exception 'third challenge creation should succeed';
    end if;

    result := public.confirm_discord_link(fusion_user_one, repeat('c', 64));
    if result <> 'account_already_linked' then
        raise exception 'duplicate Fusion account link should be rejected, got %', result;
    end if;

    result := public.confirm_discord_link(fusion_user_two, repeat('c', 64));
    if result <> 'invalid_or_expired' then
        raise exception 'a rejected challenge must still be consumed, got %', result;
    end if;

    if public.create_discord_link_challenge('not-a-discord-id', repeat('d', 64), now() + interval '10 minutes') then
        raise exception 'invalid Discord IDs must be rejected';
    end if;

    if not public.create_discord_link_challenge(discord_three, repeat('e', 64), now() + interval '10 minutes')
       or not public.create_discord_link_challenge(discord_three, repeat('f', 64), now() + interval '10 minutes')
       or not public.create_discord_link_challenge(discord_three, repeat('0', 64), now() + interval '10 minutes') then
        raise exception 'the first three requests should be allowed';
    end if;

    if public.create_discord_link_challenge(discord_three, repeat('1', 64), now() + interval '10 minutes') then
        raise exception 'the fourth request in fifteen minutes must be rate-limited';
    end if;

    if has_table_privilege('anon', 'public.discord_link_challenges', 'select')
       or has_table_privilege('authenticated', 'public.discord_link_challenges', 'select')
       or has_table_privilege('anon', 'public.discord_account_links', 'select')
       or has_table_privilege('authenticated', 'public.discord_account_links', 'select') then
        raise exception 'client roles must not read private linking tables';
    end if;

    if not has_table_privilege('service_role', 'public.discord_link_challenges', 'select')
       or not has_table_privilege('service_role', 'public.discord_account_links', 'select') then
        raise exception 'service_role must have backend table access';
    end if;
end;
$$;

rollback;
