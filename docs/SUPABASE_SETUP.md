# Supabase setup and deployment

This guide configures the current Fusion Installer account/catalog foundation.
Use a dedicated Supabase project for development and test migrations against a
disposable project before using production data.

## 1. Configure the project

1. Create a Supabase project.
2. Keep email/password authentication enabled if the installer account screen
   will be used.
3. Copy the project URL and the publishable/anon client key.
4. Configure these environment variables for the Windows process:

   - `FUSION_SUPABASE_URL`: project URL, for example
     `https://your-project.supabase.co`.
   - `FUSION_SUPABASE_ANON_KEY`: the project's publishable/anon key.

Do not put a service-role key in the installer, repository, workflow artifacts,
or any other client-side application. The desktop app is an untrusted client;
row-level security must enforce every user-facing permission.

## 2. Apply database migrations

Apply the SQL files in order:

1. `supabase/migrations/0001_initial.sql`
2. `supabase/migrations/0002_catalog_releases_reviews.sql`

You can apply them through the Supabase SQL Editor or the Supabase CLI using
your own linked project. Do not skip migration 0002: it adds release metadata,
reviews, and download-event records.

For a CLI workflow, install the Supabase CLI, authenticate locally, link the
intended project, review the pending migration list, and then push migrations.
Do not commit local access tokens or project secrets.

## 3. Current permissions and capabilities

- Profiles and sync data are scoped to the signed-in user by RLS.
- Authenticated users can read approved catalog entries.
- Users can submit mods and read their own submissions.
- Reviews are limited to one per user per mod; ratings are 1–5 and review text
  is limited to 2,000 characters. Users can edit only their own rating/body.
- Review visibility/moderation fields are not client-writable.
- Users can record download events only for published releases of approved mods.
- Release metadata is readable only after a release is published and its mod is
  approved. Clients cannot insert or edit release metadata.

## 4. Important release-security limitation

Migration 0002 is metadata infrastructure, not a working package distribution
service. Release rows are restricted to package format 3 and start as
`pending`. The native loader does not yet verify Ed25519 signatures or consume
an authenticated publisher-key registry.

Until those capabilities and a trusted server-side publishing workflow exist:

- Do not publish release rows.
- Do not create a public Storage bucket for mod packages.
- Do not expose permanent artifact URLs or mint signed download URLs.
- Do not treat format 2 SHA-256 hashes as publisher authentication.
- Keep publisher review, key rotation, revocation, and release publication on a
  trusted server—not in the Windows client.

The installer currently supports registration, sign-in, in-memory access-token
refresh, sign-out, and reading approved catalog entries. It does not yet
download or install catalog packages.

## 5. Validation checklist

Before enabling a project for real users:

- Apply both migrations to a disposable Supabase project.
- Verify an unauthenticated request cannot read authenticated catalog data.
- Verify one user cannot read or modify another user's profile, sync data,
  submission, review, or download events.
- Verify a user cannot change review moderation fields.
- Verify clients cannot insert or update release metadata.
- Verify a download event cannot be recorded for a pending/yanked release or an
  unapproved mod.
- Verify service-role credentials are absent from the installer and repository.
- Re-run these checks after any RLS or grants change.

Use forward migrations for schema changes after a migration has been applied;
do not edit an already-applied migration in a deployed project.
