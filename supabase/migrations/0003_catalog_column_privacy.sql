-- Reduce the catalog fields visible to authenticated desktop clients.
-- Keep storage object paths and user identifiers server-side.

revoke select on public.mod_releases from authenticated;
grant select (
    id,
    mod_id,
    version,
    channel,
    package_format_version,
    publisher_id,
    publisher_key_id,
    manifest_sha256,
    status,
    published_at
) on public.mod_releases to authenticated;

revoke select on public.mod_reviews from authenticated;
grant select (
    id,
    mod_id,
    rating,
    body,
    created_at,
    updated_at
) on public.mod_reviews to authenticated;

revoke select on public.mod_download_events from authenticated;
grant select (
    id,
    release_id,
    event_type,
    created_at
) on public.mod_download_events to authenticated;
