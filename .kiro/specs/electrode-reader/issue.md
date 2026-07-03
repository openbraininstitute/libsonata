# Add Electrode File Reader to libsonata

## Problem

The LFP electrode weights file (produced by BlueRecording) is currently read by an
ad-hoc reader in neurodamus (`neurodamus/io/lfp_reader.py`). This reader:

- Does a linear scan (`np.where(node_ids == gid)`) per GID lookup — O(n) per cell
- Has no population discovery (caller must know the population name)
- Lives in the wrong layer (simulation engine, not I/O library)

The reader belongs in libsonata — the SONATA I/O library — alongside `ReportReader`
and `SpikeReader`.

## Electrode File Format (source of truth)

Spec: https://sonata-extension.readthedocs.io/en/latest/sonata_tech.html#format-of-the-electrodes-file

HDF5 layout:

| Path | Type | Shape | Description |
|------|------|-------|-------------|
| `/{population}/node_ids` | uint | N_nodes | Node IDs present |
| `/{population}/offsets` | uint | N_nodes+1 | Per-node compartment boundaries |
| `/electrodes/{population}/scaling_factors` | float64 | (Total_comp, N_elec) | Scaling factors (mV/nA) |
| `/electrodes/{electrodename}/position` | float32 | 3 | Position in µm |
| `/electrodes/{electrodename}/type` | utf8 | 1 | Equation type |
| `/electrodes/{electrodename}/{population}` | uint | 1 | Column index in scaling_factors |
| `/electrodes/{electrodename}/layer` | utf8 | 1 | Optional |
| `/electrodes/{electrodename}/region` | utf8 | 1 | Optional |

**Node IDs are NOT guaranteed sorted.** BlueRecording writes in MPI rank-order for
performance. libsonata's `ReportReader` already handles unsorted node_ids via an
internal sorted index — the electrode reader should do the same.

## Requirements

1. The reader MUST discover populations from the file (no external assumptions).
2. The reader MUST handle unsorted `node_ids` via an internal index (O(log n) lookup).
3. The reader MUST support Selection-based access for both node_ids and electrode indices.
4. The reader MUST expose electrode metadata (names, positions, types).
5. The reader MUST follow libsonata naming conventions and existing patterns
   (`ReportReader`, `SpikeReader`).
6. The reader is read-only. Writing remains in BlueRecording.
7. The reader MUST have C++ implementation with Python bindings (pybind11).
8. `nullopt` Selection means "all" (existing libsonata policy).
9. The file is still named `electrode_file` — no breaking changes to the naming.

## Migration Path

1. Implement `ElectrodeReader` in libsonata (C++ + Python bindings)
2. neurodamus replaces `LFPFileReader` with `libsonata.ElectrodeReader`
