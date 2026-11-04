# Required DOS files

See [Where to obtain the games](GETTING_THE_GAMES.md) for purchase/shareware links. Availability and converter compatibility are separate: the tool validates matching executable and resource SHA-256 hashes before producing a pack.

Copy each complete episode directory from one DOS version. Select all desired episodes when generating a pack; an existing pack is replaced, not merged. Missing episodes cannot launch. Configuration and save files are unnecessary.

Keen 1–5 profiles match the verified Steam DOS collection. Keen 6 requires the verified full EGA resources paired with KEEN6C.EXE; that filename alone does not imply CGA support. Keen 1–3 require all nineteen listed world/title/special/level files.

For Dreams, prefer **v1.93 retail** from Steam Definitive Edition's **DOS/EGA** folder. **v1.13 shareware** and existing **v1.00** sets are also supported. Select only one Dreams revision per pack. Native remake and DOS/CGA files are not interchangeable. Back up Saves/keen before changing Dreams revisions; their saves are incompatible.

## Quick reference

| Episode | Required files | Validated profiles |
| --- | --- | --- |
| Keen 1 | 31 | Verified DOS set |
| Keen 2 | 24 | Verified DOS set |
| Keen 3 | 24 | Verified DOS set |
| Keen 4 | 4 | Verified DOS set |
| Keen 5 | 4 | Verified DOS set |
| Keen 6 | 4 | Verified DOS set |
| Keen Dreams | 4 / 6 / 5 | v1.00 / v1.13 / v1.93 EGA |

## Keen 1 - Marooned on Mars

```text
KEEN1.EXE
EGAHEAD.CK1
EGALATCH.CK1
EGASPRIT.CK1
LEVEL01.CK1
LEVEL02.CK1
LEVEL03.CK1
LEVEL04.CK1
LEVEL05.CK1
LEVEL06.CK1
LEVEL07.CK1
LEVEL08.CK1
LEVEL09.CK1
LEVEL10.CK1
LEVEL11.CK1
LEVEL12.CK1
LEVEL13.CK1
LEVEL14.CK1
LEVEL15.CK1
LEVEL16.CK1
LEVEL80.CK1
LEVEL81.CK1
LEVEL90.CK1
FINALE.CK1
SOUNDS.CK1
HELPTEXT.CK1
STORYTXT.CK1
ENDTEXT.CK1
PREVIEW2.CK1
PREVIEW3.CK1
PREVIEWS.CK1
```

KEEN1.EXE SHA-256: `8B3F829F7EEAB18A7FBF9D539967E9AE9C856258AA78EFC0531FA922DF71C4A2`.

## Keen 2 - The Earth Explodes

```text
KEEN2.EXE
EGAHEAD.CK2
EGALATCH.CK2
EGASPRIT.CK2
LEVEL01.CK2
LEVEL02.CK2
LEVEL03.CK2
LEVEL04.CK2
LEVEL05.CK2
LEVEL06.CK2
LEVEL07.CK2
LEVEL08.CK2
LEVEL09.CK2
LEVEL10.CK2
LEVEL11.CK2
LEVEL12.CK2
LEVEL13.CK2
LEVEL14.CK2
LEVEL15.CK2
LEVEL16.CK2
LEVEL80.CK2
LEVEL81.CK2
LEVEL90.CK2
FINALE.CK2
```

KEEN2.EXE SHA-256: `01A05A1584C4245DBF14B4DE74BCC10205D4B0944B891AD9B24D01A33F93B966`.

## Keen 3 - Keen Must Die!

```text
KEEN3.EXE
EGAHEAD.CK3
EGALATCH.CK3
EGASPRIT.CK3
LEVEL01.CK3
LEVEL02.CK3
LEVEL03.CK3
LEVEL04.CK3
LEVEL05.CK3
LEVEL06.CK3
LEVEL07.CK3
LEVEL08.CK3
LEVEL09.CK3
LEVEL10.CK3
LEVEL11.CK3
LEVEL12.CK3
LEVEL13.CK3
LEVEL14.CK3
LEVEL15.CK3
LEVEL16.CK3
LEVEL80.CK3
LEVEL81.CK3
LEVEL90.CK3
FINALE.CK3
```

KEEN3.EXE SHA-256: `CE240FEE9E23E0C359492CE3D247A29CD2309B1B9822D0267E587DF5838C9047`.

## Keen 4 - Secret of the Oracle

```text
KEEN4E.EXE
EGAGRAPH.CK4
GAMEMAPS.CK4
AUDIO.CK4
```

KEEN4E.EXE SHA-256: `BDBB46746E02C0DDDCBA9B24FA464C26CC7E3514A3B3931267C8C17B9FB9C405`.

## Keen 5 - The Armageddon Machine

```text
KEEN5E.EXE
EGAGRAPH.CK5
GAMEMAPS.CK5
AUDIO.CK5
```

KEEN5E.EXE SHA-256: `77F687268BE09763D139A1226A0F4EEC191E62181299FF466D54FC232670CF57`.

## Keen 6 - Aliens Ate My Babysitter!

```text
KEEN6C.EXE
EGAGRAPH.CK6
GAMEMAPS.CK6
AUDIO.CK6
```

KEEN6C.EXE SHA-256: `050DADF0C1902FCF9C7288E2A704057BD5EC2159388D3900A0F13AB326B134A3`.

## Keen Dreams EGA DOS v1.93

```text
KDREAMS.AUD
KDREAMS.CMP
KDREAMS.EGA
KDREAMS.EXE
KDREAMS.MAP
```

KDREAMS.EXE SHA-256: `E4D39895561AA6854A52FDFE90AC2D9DE47CA296DEEE7F2900918DB34605780D`.

## Keen Dreams EGA DOS v1.13

```text
KDREAMS.AUD
KDREAMS.CMP
KDREAMS.EGA
KDREAMS.EXE
KDREAMS.MAP
LAST.SHL
```

KDREAMS.EXE SHA-256: `574D49B882AF9AC15232EADB4C6EDCE94A35B30D622F6E01F6AE20565F7C5AAB`.

## Keen Dreams EGA DOS v1.00

```text
AUDIO.KDR
EGAGRAPH.KDR
GAMEMAPS.KDR
KDREAMS.EXE
```

KDREAMS.EXE SHA-256: `36A22DBD3119C0B4CDFF7ADAF50A8ABC3B11C21F7BC0D4D7C0D5DBF7B347E3AE`.

## Convert and install

Extract **RetroDriven.CommanderKeen-GamePrep-v1.0.0-Windows.zip** on your PC and open the EXE. Add the DOS episode folders, choose an output folder and optionally select the Pocket SD root. Click **Prepare game data**. The prepared folder contains **Assets/keen/common/keenpack.bin** and can be merged into the SD root after installing the public core ZIP.

The app checks executable and resource hashes, then creates and validates the pack before copying it to the SD. Missing or mismatched files are reported in the window/log. Originals are read without modification.

For the linked Dreams shareware download, extract the outer ZIP and nested **keendrms.zip**, then select the folder containing KDREAMS.EXE. Keep the original shareware license/readme with your copy.