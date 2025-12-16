import re

class InputValidator:
    """Třída pro validaci vstupů v lobby."""
    
    @staticmethod
    def validate_nickname(nickname: str) -> tuple[bool, str]:
        """
        Validuje nickname.
        
        Pravidla:
        - Max 12 znaků
        - Pouze písmena a číslice (A-Z, a-z, 0-9)
        - Neprázdný
        
        Returns:
            tuple[bool, str]: (je_validní, chybová_zpráva)
        """
        if not nickname or nickname.strip() == "":
            return False, "Nickname nesmí být prázdný!"
        
        if len(nickname) > 12:
            return False, "Nickname může mít max 12 znaků!"
        
        # Pouze alfanumerické znaky (A-Z, a-z, 0-9)
        if not re.match(r'^[A-Za-z0-9]+$', nickname):
            return False, "Nickname může obsahovat pouze písmena a číslice!"
        
        return True, ""
    
    @staticmethod
    def validate_port(port_str: str) -> tuple[bool, str, int]:
        """
        Validuje port.
        
        Pravidla:
        - Číslo v rozsahu 1024-65535
        
        Returns:
            tuple[bool, str, int]: (je_validní, chybová_zpráva, port_číslo)
        """
        if not port_str or port_str.strip() == "":
            return False, "Port nesmí být prázdný!", 0
        
        try:
            port = int(port_str)
        except ValueError:
            return False, "Port musí být číslo!", 0
        
        if port < 1024 or port > 65535:
            return False, "Port musí být v rozsahu 1024-65535!", 0
        
        return True, "", port
    
    @staticmethod
    def validate_ip(ip: str) -> tuple[bool, str]:
        """
        Validuje IPv4 adresu.
        
        Pravidla:
        - Formát xxx.xxx.xxx.xxx
        - Každé xxx je číslo 0-255
        
        Returns:
            tuple[bool, str]: (je_validní, chybová_zpráva)
        """
        if not ip or ip.strip() == "":
            return False, "IP adresa nesmí být prázdná!"
        
        # Regex pro validaci IPv4
        ip_pattern = r'^(\d{1,3})\.(\d{1,3})\.(\d{1,3})\.(\d{1,3})$'
        match = re.match(ip_pattern, ip)
        
        if not match:
            return False, "Neplatný formát IP! (xxx.xxx.xxx.xxx)"
        
        # Zkontroluj, že každá část je 0-255
        for octet_str in match.groups():
            octet = int(octet_str)
            if octet < 0 or octet > 255:
                return False, f"Neplatná IP! Číslo {octet} musí být 0-255"
        
        return True, ""
    
    @staticmethod
    def validate_all(ip: str, port_str: str, nickname: str) -> tuple[bool, str, int]:
        """
        Validuje všechny vstupy najednou.
        
        Returns:
            tuple[bool, str, int]: (je_vše_validní, chybová_zpráva, port_číslo)
        """
        # Validace nickname
        nick_valid, nick_error = InputValidator.validate_nickname(nickname)
        if not nick_valid:
            return False, nick_error, 0
        
        # Validace IP
        ip_valid, ip_error = InputValidator.validate_ip(ip)
        if not ip_valid:
            return False, ip_error, 0
        
        # Validace portu
        port_valid, port_error, port_num = InputValidator.validate_port(port_str)
        if not port_valid:
            return False, port_error, 0
        
        return True, "", port_num