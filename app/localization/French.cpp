#include "StringIDs.h"
#include <exception>
#include "Languages.h"

using namespace std;
using namespace string_literals;

namespace Localization::French
{
	string GetString(int id)
	{
		switch (id)
		{
		case StringID::OK:
			return "OK"s;

		case StringID::CANCEL:
			return "Annuler"s;

		case StringID::CLOSE:
			return "Fermer"s;

		case StringID::BONJOUR_INSTALL:
#ifdef _WIN32
			return R"(<p>Veuillez installer Bonjour pour Windows depuis ce lien :</p>
                      <p><a href="https://support.apple.com/kb/DL999">https://support.apple.com/kb/DL999</a></p>)"s;
#else
			return "Veuillez installer avahi-daemon et libavahi-compat-libdnssd-dev"s;
#endif
		case StringID::TROUBLE_SHOOT_RAOP_SERVICE:
			return "Le service AirPlay n'a pas pu d\xC3\xA9marrer.\nVeuillez activer votre service Bonjour/Avahi."s;

		case StringID::RECONFIG_RAOP_SERVICE:
			return "Le flux en cours sera interrompu pour la reconfiguration, d\xC3\xA9sol\xC3\xA9 !"s;

		case StringID::FAILED_TO_START_DACP_BROWSER:
			return "Le navigateur DACP n'a pas pu d\xC3\xA9marrer. Le contr\xC3\xB4le multim\xC3\xA9""dia n'est pas disponible."s;

		case StringID::MENU_FILE:
			return "&Fichier"s;

		case StringID::MENU_QUIT:
			return "&Quitter"s;

		case StringID::MENU_EDIT:
			return "\xC3\x89&dition"s;

		case StringID::MENU_OPTIONS:
			return "&Options..."s;

		case StringID::MENU_HELP:
			return "&Aide"s;

		case StringID::MENU_ABOUT:
			return "\xC3\x80 &propos..."s;

		case StringID::LABEL_AIRPORT_NAME:
			return "Nom Airport"s;

		case StringID::LABEL_AIRPORT_PASSWORD:
			return "Mot de passe"s;

		case StringID::LABEL_AIRPORT_CHANGE:
			return "Modifier..."s;

		case StringID::LABEL_TITLE_INFO:
			return "Info piste"s;

		case StringID::TOOLTIP_PREV_TRACK:
			return "Pr\xC3\xA9""c\xC3\xA9""dent"s;

		case StringID::TOOLTIP_NEXT_TRACK:
			return "Suivant"s;

		case StringID::TOOLTIP_VOLUME_UP:
			return "Volume +"s;

		case StringID::TOOLTIP_VOLUME_DOWN:
			return "Volume -"s;

		case StringID::TOOLTIP_PLAY_PAUSE:
			return "Lecture/Pause"s;

		case StringID::STATUS_READY:
			return "Pr\xC3\xAAt"s;

		case StringID::STATUS_CONNECTED:
			return "Connect\xC3\xA9 \xC3\xA0 "s;

		case StringID::DIALOG_CHANGE_NAME_PASSWORD:
			return "Modifier le nom Airport et le mot de passe"s;

		case StringID::DIALOG_ABOUT:
			return "\xC3\x80 propos"s;

		case StringID::ABOUT_INFO:
			return CW2AEX(L"\xA9"s) + " Copyright 2026\nFrank Friemel\n\nShairportQt est bas\xC3\xA9 sur Shairport de James Laird\n "s;

		case StringID::OPTION_MINIMIZED:
			return "D\xC3\xA9marrer r\xC3\xA9""duit"s;

		case StringID::OPTION_AUTOSTART:
			return "D\xC3\xA9marrage automatique"s;

		case StringID::DIALOG_OPTIONS:
			return "Options avanc\xC3\xA9""es"s;

		case StringID::LABEL_DATA_BUFFER:
			return "Mise en m\xC3\xA9moire tampon"s;

		case StringID::LABEL_MIN:
			return "min"s;

		case StringID::LABEL_MAX:
			return "max"s;

		case StringID::LABEL_SOUND_DEVICE:
			return "P\xC3\xA9riph\xC3\xA9rique audio"s;

		case StringID::LABEL_LOG_TO_FILE:
			return "Journaliser dans un fichier"s;

		case StringID::LABEL_DISABLE_MM_CONTROL:
			return "D\xC3\xA9sactiver les contr\xC3\xB4les multim\xC3\xA9""dia"s;

		case StringID::LABEL_DEFAULT_DEVICE:
			return "P\xC3\xA9riph\xC3\xA9rique par d\xC3\xA9""faut du syst\xC3\xA8me"s;

		case StringID::LABEL_KEEP_STICKY:
			return "Fen\xC3\xAAtre toujours visible"s;

		case StringID::LABEL_TRAY_ICON:
			return "Ic\xC3\xB4ne de notification"s;

		case StringID::MENU_SHOW_APP_WINDOW:
			return "A&fficher"s;

		case StringID::MENU_SHOW_TRACK_INFO_IN_TRAY:
			return "&Afficher \"En cours de lecture\" dans la zone de notification"s;

		}
		return English::GetString(id);
	}
}
