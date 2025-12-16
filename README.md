# Documentation for UPS project

Tento repozitář obsahuje semestrální práci k předmětu **UPS** (Úvod do počítačových sítí) na téma **síťová multiplayer hra Mariáš**.

## Mariáš – Multiplayer Game (UPS Project)

Tento projekt vznikl jako **semestrální práce** v rámci předmětu **UPS**.

⚠️ **Upozornění:**  
Projekt byl vytvořen pro studijní účely. Nejedná se o produkční software.

- Autor: David Wimmer
- Rok: 2025
- Účel: studijní a výukový

Kód může být použit v souladu s licencí MIT, avšak **není garantována správnost, bezpečnost ani úplnost řešení**.
Projekt není oficiální implementací hry Mariáša nebyl konzultován s žádnou institucí mimo výuku.

## Licence

Tento projekt je distribuován pod licencí **MIT**.  
Viz soubor [LICENSE](LICENSE).

### Struktura práce
```bash
|- docs 
|-- server    # Všechny soubory pro běh serveru
  |- makefile # Dokument pro automatickou kompilaci programu 
  |- game     # Soubory pro logiku hry
  |- builtor  # Simulace běhu serveru
  ...
|-|
|-- client    # Všechny soubory pro běh klienta
  |- main.py  # Soubor pro spuštění klienta
  |- src      # Logika klienta
  |- images   # Obrázky karet
|-|
```

### Spuštění
**Technické požadavky**
- Interpret jazyka Python
- Kompilační nástroj g++

**Client**
*Mělo by fungovat i bez virtuálního prostředí*

- Windows
```bash
> cd UPS/client
> python -m venv .venv
> ./.venv/Scripts/activate
> pip install pygame
> python main.py
```
- Linux
```bash
> cd UPS/client
> python -m venv .venv
> source .venv/bin/activate
> pip install pygame
> python main.py
```

*Server*
- Linux
```bash
> cd UPS/server
> make
> ./marias.exe
Pro zobrazení nápovědy
> ./marias.exe -h
```

