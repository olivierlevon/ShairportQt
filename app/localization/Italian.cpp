#include "StringIDs.h"
#include <exception>
#include "Languages.h"

using namespace std;
using namespace string_literals;

namespace Localization::Italian
{
	string GetString(int id)
	{
		switch (id)
		{
		case StringID::OK:
			return "OK"s;

		case StringID::CANCEL:
			return "Annulla"s;

		case StringID::CLOSE:
			return "Chiudi"s;

		case StringID::BONJOUR_INSTALL:
#ifdef _WIN32
			return R"(<p>Installare Bonjour per Windows:</p>
                      <ul>
                      <li>Con <a href="https://community.chocolatey.org/packages/bonjour/">Chocolatey</a>: <code>choco install bonjour</code></li>
                      <li>Oppure installare <a href="https://www.apple.com/itunes/download/win64/">iTunes per Windows</a> (versione desktop, non quella del Microsoft Store)</li>
                      </ul>)"s;
#else
			return "Installare avahi-daemon e libavahi-compat-libdnssd-dev"s;
#endif
		case StringID::TROUBLE_SHOOT_RAOP_SERVICE:
			return "Il servizio AirPlay non ha potuto avviarsi.\nAbilitare il servizio Bonjour/Avahi."s;

		case StringID::RECONFIG_RAOP_SERVICE:
			return "Lo streaming in corso verr\xC3\xA0 interrotto per la riconfigurazione, spiacenti!"s;

		case StringID::FAILED_TO_START_DACP_BROWSER:
			return "Il browser DACP non ha potuto avviarsi. Il controllo multimediale non \xC3\xA8 disponibile."s;

		case StringID::MENU_FILE:
			return "&File"s;

		case StringID::MENU_QUIT:
			return "&Esci"s;

		case StringID::MENU_EDIT:
			return "&Modifica"s;

		case StringID::MENU_OPTIONS:
			return "&Opzioni..."s;

		case StringID::MENU_HELP:
			return "&Aiuto"s;

		case StringID::MENU_ABOUT:
			return "I&nformazioni..."s;

		case StringID::LABEL_AIRPORT_NAME:
			return "Nome Airport"s;

		case StringID::LABEL_AIRPORT_PASSWORD:
			return "Password"s; // same in Italian

		case StringID::LABEL_AIRPORT_CHANGE:
			return "Modifica..."s;

		case StringID::LABEL_TITLE_INFO:
			return "Info traccia"s;

		case StringID::TOOLTIP_PREV_TRACK:
			return "Indietro"s;

		case StringID::TOOLTIP_NEXT_TRACK:
			return "Avanti"s;

		case StringID::TOOLTIP_VOLUME_UP:
			return "Volume +"s;

		case StringID::TOOLTIP_VOLUME_DOWN:
			return "Volume -"s;

		case StringID::TOOLTIP_PLAY_PAUSE:
			return "Riproduci/Pausa"s;

		case StringID::STATUS_READY:
			return "Pronto"s;

		case StringID::STATUS_CONNECTED:
			return "Connesso a "s;

		case StringID::DIALOG_CHANGE_NAME_PASSWORD:
			return "Modifica nome Airport e password"s;

		case StringID::DIALOG_ABOUT:
			return "Informazioni"s;

		case StringID::ABOUT_INFO:
			return CW2AEX(L"\xA9"s) + " Copyright 2026\nFrank Friemel\n\nShairportQt \xC3\xA8 basato su Shairport di James Laird\n "s;

		case StringID::OPTION_MINIMIZED:
			return "Avvia ridotto a icona"s;

		case StringID::OPTION_AUTOSTART:
			return "Avvio automatico"s;

		case StringID::DIALOG_OPTIONS:
			return "Opzioni avanzate"s;

		case StringID::LABEL_DATA_BUFFER:
			return "Buffering"s;

		case StringID::LABEL_MIN:
			return "min"s;

		case StringID::LABEL_MAX:
			return "max"s;

		case StringID::LABEL_SOUND_DEVICE:
			return "Dispositivo audio"s;

		case StringID::LABEL_LOG_TO_FILE:
			return "Salva log su file"s;

		case StringID::LABEL_DISABLE_MM_CONTROL:
			return "Disabilita controlli multimediali"s;

		case StringID::LABEL_DEFAULT_DEVICE:
			return "Dispositivo predefinito di sistema"s;

		case StringID::LABEL_KEEP_STICKY:
			return "Finestra sempre in primo piano"s;

		case StringID::LABEL_TRAY_ICON:
			return "Icona nell'area di notifica"s;

		case StringID::MENU_SHOW_APP_WINDOW:
			return "M&ostra"s;

		case StringID::MENU_SHOW_TRACK_INFO_IN_TRAY:
			return "&Mostra \"In riproduzione\" nell'area di notifica"s;

		}
		return English::GetString(id);
	}
}
