# ADR-002 — Resource system refactor

- **Status:** Accepted
- **Date:** 2026-07
- **Deciders:** Miguel (Lead Engineer)

## Context

Resource handling (meshes, models, textures, materials, scenes, shaders) had
grown inconsistent. Shaders in particular were managed by a bespoke
`ShadersManager` separate from the rest of the resource pipeline, duplicating
lifecycle logic and complicating editor loading.

## Decision

Consolidate all resource types under a single `ResourceSystem` built on two
interfaces:

- `IResourceCache` — owns/stores loaded resources of a type.
- `IResourceLoader` — turns files into resources.

Each type (material, mesh, model, scene, shader, texture) lives in its own
subfolder under `resources/`. Shaders become a first-class resource
(`ShaderResource`) loaded via `ShadersLoader`, with lifecycle signalled through
events (`ShaderCreatedEvent`, `ShaderDeletedEvent`). The old `ShadersManager`
is removed.

## Consequences

**Positive**
- One consistent path for every resource type; less duplicated lifecycle code.
- Shaders integrate with the editor's loader flow like any other asset.
- Event-driven lifecycle lets the renderer react to shader create/delete.

**Negative / open**
- Migration touched many files (client renderer, editor, templates); watch for
  stragglers still assuming the old shader API.
- Cache eviction / hot-reload policy is not yet specified — future ADR if needed.

## Alternatives considered

- **Keep `ShadersManager` separate** — rejected: perpetuates two systems.
- **Fully generic templated resource manager** — deferred: per-type
  loaders/caches are clearer to debug for now.
