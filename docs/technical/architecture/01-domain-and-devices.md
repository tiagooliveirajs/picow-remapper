# Domain and device model

## Identity and registry

A saved device is identified by a stable internal `DeviceId` plus transport identity. Display names are presentation data and must not be used as identity keys.

Conceptually:

```text
SavedDevice
- DeviceId
- DeviceType: Mouse | Keyboard | Composite
- TransportKind: BLE_HOGP | CLASSIC_HID
- DeviceIdentity
- display_name
- pairing/bond reference
- MouseProfile (Mouse only)
- capability/quirk metadata
```

The registry persists many devices per type.

## Preferred devices

Preferred devices are references into the saved registry, one optional reference per type. They represent reconnect priority, not storage ownership.

## Active slots

Runtime has three independent slots:

```text
ActiveSlots
- Mouse: optional DeviceId
- Keyboard: optional DeviceId
- Composite: optional DeviceId
```

At most one peer is active in each slot, but multiple saved peers may remain inactive.

## Connection Coordinator

The coordinator owns high-level connection policy: reconnect preferred peers, consider saved fallback peers, request pairing, switch active peer, and remove peer. Screens emit application commands; they do not drive transport state machines directly.

## Pair transaction

New pairing follows a prepare/commit shape. Existing saved/active data remains valid until the candidate has authenticated, been classified as the requested type, and its credentials/record have been persisted successfully.

## Remove transaction

Removing a device coordinates:

- disconnect if active;
- release only that source's HID ownership;
- delete BLE bond or Classic link key/peer credentials;
- delete product `SavedDevice` record;
- delete per-mouse profile and quirk/configuration state when present;
- clear preferred reference when it points to that device;
- persist atomically enough that power failure does not leave a falsely successful UI state.

## Persistence boundary

Product state and Bluetooth credentials are distinct stores with a coordinating repository/service boundary. Product records carry a storage schema/version and integrity mechanism (for example generation + CRC/journal strategy) rather than relying on incidental BTstack record layout.

## Mouse profiles

Profiles are owned by saved Mouse records. Switching active Mouse selects that mouse's profile. New Mouse records default to Passthrough.

Custom editing uses `CustomDraft` separate from `AppliedProfile`; per-button `APPLY THIS` edits the draft, while final custom apply validates/persists the complete draft.
