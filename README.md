# GPS_IsDogOutside
# MODUŁ GPS - LOKALIZACJA ZWIERZĄT
## Ewa Krzaczkowska

Celem projektu jest urządzenie sprawujące wirtualną opiekę nad  	zwierzęciem w zakresie monitorowania jego położenia i wysyłanie 	komunikatu jeśli przekroczył dozwolony dla niego obszar.  
Mikrokontroler  odczytuje sygnały z modułu GPS, po czym przelicza 	zebrane dane na położenie i sprawdza czy obiekt znajduje się w 	wyznaczonym obszarze. Wyniki pomiarów przekazuje do użytkownika.

Sprzęt:
1. [x] Moduł GPS FGPMMOPA6C 
2. [x] Płytka NUCLEO STM32F401RE

Założenia
1. [x] Odczytywanie informacji o położeniu co 1 sekundę.
2. [x] Skupienie się na funkcjonalności bez uwzględnienia procesu miniaturyzacji (makieta mikroprocesorowa).
3. [x] Symulacja użycia modemu radiowego poprzez wyprowadzenie na wyjście TX informacji przesyłanych w przypadku uproszczonego projektu na komputer.  

Budowa zdania NMEA (wykorzystujemy sekwencje RMC):  
Zaczyna się znakiem „$”,  
identyfikator urządzenia nadającego oraz trzyliterowe hasło  
nagłówek,  
informacje (oddzielone od siebie przecinkami, każda o określonym miejscu w zdaniu),  
na końcu znak ‘*’ oraz suma kontrolna.  

Zaimplementowanie symulatora modułu sterowania modem poprzez interfejs szeregowy (USART ang. Universal Synchronous and Asynchronous Receiver and Transmitter) z założeniem:  
 prędkości – 115200,  
 8 bitów danych,  
 brak kontroli parzystości,  
 bit stopu.  
 
 ![image](https://user-images.githubusercontent.com/112576532/211844268-4f777b6c-536f-45ba-9bbe-f027a77c51fd.png)

Wirtualnym ogrodzeniem jest obszar podwórka właściciela czworonoga, jeżeli zwierzę z nieznanych przyczyn opuści ten teren konieczne jest wysłanie o tym informacja do właściciela. 

Projekt w trakcie realizacji
