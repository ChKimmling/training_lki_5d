# Exkurs: SLAB / SLUB im Linux-Kernel

## Worum geht es?

Der Linux-Kernel benötigt sehr häufig kleine Speicherblöcke für interne Objekte, zum Beispiel:

- `task_struct`
- Inodes
- Dentries
- Netzwerkstrukturen
- Treiberobjekte

Dafür wäre es ineffizient, jedes Objekt einzeln direkt aus dem Page Allocator zu holen.

Deshalb verwendet Linux sogenannte **Object Allocators**.

```text
Buddy / Page Allocator
        |
        v
   SLAB / SLUB
        |
        v
kleine Kernelobjekte
```

## Grundidee

SLAB und SLUB verwalten Speicher in **Caches für Objekttypen**.

Ein Cache enthält viele Objekte gleicher Größe.

```text
kmem_cache
   |
   +--> Objekt
   +--> Objekt
   +--> Objekt
```

Das reduziert Verwaltungsaufwand und Fragmentierung.

## SLAB

**SLAB** ist der klassische Linux-Allocator.

Er organisiert Speicher grob in:

```text
Cache
  |
  +--> Slab
         |
         +--> Objekt
         +--> Objekt
         +--> Objekt
```

Ein Slab besteht aus einer oder mehreren Pages und enthält mehrere gleichartige Objekte.

## SLUB

**SLUB** ist eine modernere und einfachere Implementierung.

Ziele:

- weniger Metadaten
- einfacherer Code
- bessere Skalierbarkeit
- gute Performance auf SMP-Systemen

Heute ist SLUB auf vielen Linux-Systemen der Standard.

## Cache und Slab

Ein typisches Modell:

```text
kmem_cache
   |
   +--> Slab / Page
          |
          +--> Objekt A
          +--> Objekt B
          +--> Objekt C
```

Ein Cache ist für einen bestimmten Objekttyp oder eine bestimmte Objektgröße gedacht.

## Eigene Caches

Kernel-Code kann eigene Caches anlegen:

```c
cache = kmem_cache_create(
    "my_cache",
    sizeof(struct my_object),
    0,
    0,
    NULL
);
```

Objekt anfordern:

```c
obj = kmem_cache_alloc(cache, GFP_KERNEL);
```

Objekt freigeben:

```c
kmem_cache_free(cache, obj);
```

## `kmalloc()` und SLUB

Auch `kmalloc()` verwendet intern Größenklassen des SLAB/SLUB-Allocators.

Beispiel:

```c
ptr = kmalloc(100, GFP_KERNEL);
```

Intern landet der Request typischerweise in einem passenden Cache, zum Beispiel:

```text
kmalloc-128
```

Vereinfacht:

```text
kmalloc(100)
    |
    v
passender Größen-Cache
    |
    v
Objekt aus Slab
```

## Bezug zum Buddy Allocator

SLAB/SLUB arbeitet oberhalb des Page Allocators.

```text
Physischer Speicher
       |
       v
Buddy Allocator
       |
       | Pages
       v
SLAB / SLUB
       |
       | kleine Objekte
       v
Kernel-Subsysteme
```

Der Buddy Allocator verwaltet Seitenblöcke, SLAB/SLUB zerlegt diese in kleinere Objekte.

## SLAB vs. SLUB

| SLAB | SLUB |
|---|---|
| klassische Implementierung | modernere Implementierung |
| mehr Verwaltungsstrukturen | weniger Metadaten |
| komplexere Cache-Verwaltung | einfacheres Design |
| historisch weit verbreitet | heute oft Standard |
| Objekt-Caches | Objekt-Caches |

Beide verfolgen dasselbe Grundprinzip:

> Speicher für gleichartige Kernelobjekte effizient vorbereiten und wiederverwenden.

## Beobachten im System

Informationen zu Slab-Caches:

```bash
cat /proc/slabinfo
```

oder:

```bash
slabtop
```

Beispielhafte Caches:

```text
dentry
inode_cache
kmalloc-64
kmalloc-128
kmalloc-256
```

## Merksatz

> **SLAB und SLUB sind Kernel-Allocator für kleine, häufig verwendete Objekte; sie beziehen Pages vom Page Allocator, organisieren diese in Objekt-Caches und ermöglichen dadurch schnelle Allokation und Wiederverwendung.**
