#include <iostream>

#include "settings.h"
using namespace AhoViewer;

#include "config.h"

SettingsManager AhoViewer::Settings;

SettingsManager::SettingsManager()
  : Path(Glib::build_filename(Glib::get_user_config_dir(), PACKAGE, PACKAGE ".cfg")),
    BooruPath(Glib::build_filename(Glib::get_user_config_dir(), PACKAGE, "booru")),
// Defaults {{{
    DefaultBools(
    {
        { "AutoOpenArchive",      true  },
        { "MangaMode",            true  },
        { "RememberLastFile",     true  },
        { "RememberLastSavePath", true  },
        { "SaveThumbnails",       true  },
        { "StartFullscreen",      false },
        { "StoreRecentFiles",     true  },

        { "BooruBrowserVisible",  true  },
        { "MenuBarVisible",       true  },
        { "ScrollbarsVisible",    true  },
        { "StatusBarVisible",     true  },
        { "ThumbnailBarVisible",  false },
        { "HideAll",              false },
        { "HideAllFullscreen",    true  },
    }),
    DefaultInts(
    {
        { "ArchiveIndex",     -1  },
        { "CacheSize",        2   },
        { "SlideshowDelay",   5   },
        { "TagViewPosition",  560 },
        { "SelectedBooru",    0   },
        { "BooruLimit",       50  },
        { "BooruWidth",       -1  },
    }),
    DefaultSites(
    {
        std::make_tuple("Gelbooru",   "http://gelbooru.com",        Booru::Site::Type::GELBOORU),
        std::make_tuple("Danbooru",   "http://danbooru.donmai.us",  Booru::Site::Type::DANBOORU),
        std::make_tuple("Konachan",   "http://konachan.com",        Booru::Site::Type::DANBOORU),
        std::make_tuple("yande.re",   "https://yande.re",           Booru::Site::Type::DANBOORU),
        std::make_tuple("Safebooru",  "http://safebooru.org",       Booru::Site::Type::GELBOORU),
    }),
    DefaultKeybindings(
    {
        {
            "File",
            {
                { "OpenFile",            "<Primary>o" },
                { "Preferences",         "p"          },
                { "Close",               "<Primary>w" },
                { "Quit",                "<Primary>q" },
            }
        },
        {
            "ViewMode",
            {
                { "ToggleMangaMode",     "g" },
                { "AutoFitMode",         "a" },
                { "FitWidthMode",        "w" },
                { "FitHeightMode",       "h" },
                { "ManualZoomMode",      "m" },
            }
        },
        {
            "UserInterface",
            {
                { "ToggleFullscreen",    "f"          },
                { "ToggleMenuBar",       "<Primary>m" },
                { "ToggleStatusBar",     "<Primary>b" },
                { "ToggleScrollbars",    "<Primary>l" },
                { "ToggleThumbnailBar",  "t"          },
                { "ToggleBooruBrowser",  "b"          },
                { "ToggleHideAll",       "i"          },
            }
        },
        {
            "Zoom",
            {
                { "ZoomIn",              "<Primary>equal" },
                { "ZoomOut",             "<Primary>minus" },
                { "ResetZoom",           "<Primary>0"     },
            }
        },
        {
            "Navigation",
            {
                { "NextImage",           "Page_Down" },
                { "PreviousImage",       "Page_Up"   },
                { "FirstImage",          "Home"      },
                { "LastImage",           "End"       },
                { "ToggleSlideshow",     "s"         },
            }
        },
        {
            "Scroll",
            {
                { "ScrollUp",            "Up"    },
                { "ScrollDown",          "Down"  },
                { "ScrollLeft",          "Left"  },
                { "ScrollRight",         "Right" },
            }
        },
        {
            "BooruBrowser",
            {
                { "NewTab",              "<Primary>t"        },
                { "SaveImage",           "<Primary>s"        },
                { "SaveImages",          "<Primary><Shift>s" },
            }
        }
    }),
    DefaultBGColor("#161616")
// }}}
{
    Config.setTabWidth(4); // this is very important
    if (Glib::file_test(Path, Glib::FILE_TEST_EXISTS))
    {
        try
        {
            Config.readFile(Path.c_str());
        }
        catch (const libconfig::ParseException &ex)
        {
            std::cerr << "libconfig::Config.readFile: " << ex.what() << std::endl;
        }
    }

    if (!Glib::file_test(BooruPath, Glib::FILE_TEST_EXISTS))
        g_mkdir_with_parents(BooruPath.c_str(), 0700);

    load_keybindings();
}

SettingsManager::~SettingsManager()
{
    try
    {
        Config.writeFile(Path.c_str());
    }
    catch (const libconfig::FileIOException &ex)
    {
        std::cerr << "libconfig::Config.writeFile: " << ex.what() << std::endl;
    }
}

bool SettingsManager::get_bool(const std::string &key) const
{
    if (Config.exists(key))
        return Config.lookup(key);

    return DefaultBools.at(key);
}

int SettingsManager::get_int(const std::string &key) const
{
    if (Config.exists(key))
        return Config.lookup(key);

    return DefaultInts.at(key);
}

std::string SettingsManager::get_string(const std::string &key) const
{
    if (Config.exists(key))
        return (const char*)Config.lookup(key);

    return "";
}

std::vector<std::shared_ptr<Booru::Site>>& SettingsManager::get_sites()
{
    if (m_Sites.size())
    {
        return m_Sites;
    }
    else if (Config.exists("Sites"))
    {
        const Setting &sites = Config.lookup("Sites");
        if (sites.getLength() > 0)
        {
            for (size_t i = 0; i < static_cast<size_t>(sites.getLength()); ++i)
            {
                const Setting &s = sites[i];
                m_Sites.push_back(std::make_shared<Booru::Site>(s["name"], s["url"],
                            static_cast<Booru::Site::Type>(static_cast<int>(s["type"]))));
            }

            return m_Sites;
        }
    }
    else
    {
        for (const std::tuple<std::string, std::string, Booru::Site::Type> &s : DefaultSites)
            m_Sites.push_back(std::make_shared<Booru::Site>(
                        std::get<0>(s), std::get<1>(s), std::get<2>(s)));
    }

    return m_Sites;
}

void SettingsManager::update_sites()
{
    remove("Sites");
    Setting &sites = Config.getRoot().add("Sites", Setting::TypeList);

    for (const std::shared_ptr<Booru::Site> &s : m_Sites)
    {
        Setting &site = sites.add(Setting::TypeGroup);
        set("name", s->get_name(), Setting::TypeString, site);
        set("url", s->get_url(), Setting::TypeString, site);
        set("type", static_cast<int>(s->get_type()), Setting::TypeInt, site);
    }
}

bool SettingsManager::get_geometry(int &x, int &y, int &w, int &h) const
{
    if (Config.lookupValue("Geometry.x", x) && Config.lookupValue("Geometry.y", y) &&
        Config.lookupValue("Geometry.w", w) && Config.lookupValue("Geometry.h", h))
    {
        return true;
    }

    return false;
}

void SettingsManager::set_geometry(const int x, const int y, const int w, const int h)
{
    if (!Config.exists("Geometry"))
        Config.getRoot().add("Geometry", Setting::TypeGroup);

    Setting &geo = Config.lookup("Geometry");

    set("x", x, Setting::TypeInt, geo);
    set("y", y, Setting::TypeInt, geo);
    set("w", w, Setting::TypeInt, geo);
    set("h", h, Setting::TypeInt, geo);
}

std::string SettingsManager::get_keybinding(const std::string &group, const std::string &name) const
{
    return m_Keybindings.at(group).at(name);
}

/**
 * Clears the first (only) binding that has the same value as value
 * Sets the group and name parameters to those of the binding that was cleared
 * Returns true if it actually cleared a binding
 **/
bool SettingsManager::clear_keybinding(const std::string &value, std::string &group, std::string &name)
{
    for (const std::pair<std::string, std::map<std::string, std::string>> &i : m_Keybindings)
    {
        for (const std::pair<std::string, std::string> &j : i.second)
        {
            if (j.second == value)
            {
                group = i.first;
                name = j.first;

                set_keybinding(group, name, "");

                return true;
            }
        }
    }

    return false;
}

void SettingsManager::set_keybinding(const std::string &group, const std::string &name, const std::string &value)
{
    if (!Config.exists("Keybindings"))
        Config.getRoot().add("Keybindings", Setting::TypeGroup);

    Setting &keys = Config.lookup("Keybindings");

    if (!keys.exists(group))
        keys.add(group, Setting::TypeGroup);

    set(name, value, Setting::TypeString, keys[group]);
    m_Keybindings[group][name] = value;
}

std::string SettingsManager::reset_keybinding(const std::string &group, const std::string &name)
{
    if (Config.exists("Keybindings"))
    {
        Setting &keys = Config.lookup("Keybindings");

        if (keys.exists(group) && keys[group].exists(name))
            keys[group].remove(name);
    }

    return m_Keybindings[group][name] = DefaultKeybindings.at(group).at(name);
}

Gdk::Color SettingsManager::get_background_color() const
{
    if (Config.exists("BackgroundColor"))
        return Gdk::Color((const char*)Config.lookup("BackgroundColor"));

    return DefaultBGColor;
}

void SettingsManager::set_background_color(const Gdk::Color &value)
{
    set("BackgroundColor", value.to_string());
}

Booru::Site::Rating SettingsManager::get_booru_max_rating() const
{
    if (Config.exists("BooruMaxRating"))
        return Booru::Site::Rating(static_cast<int>(Config.lookup("BooruMaxRating")));

    return DefaultBooruMaxRating;
}

void SettingsManager::set_booru_max_rating(const Booru::Site::Rating value)
{
    set("BooruMaxRating", static_cast<int>(value));
}

ImageBox::ZoomMode SettingsManager::get_zoom_mode() const
{
    if (Config.exists("ZoomMode"))
        return ImageBox::ZoomMode(((const char*)Config.lookup("ZoomMode"))[0]);

    return DefaultZoomMode;
}

void SettingsManager::set_zoom_mode(const ImageBox::ZoomMode value)
{
    set("ZoomMode", std::string(1, static_cast<char>(value)));
}

void SettingsManager::remove(const std::string &key)
{
    if (Config.exists(key))
        Config.getRoot().remove(key);
}

void SettingsManager::load_keybindings()
{
    if (Config.exists("Keybindings"))
    {
        Setting &keys = Config.lookup("Keybindings");

        for (const std::pair<std::string, std::map<std::string, std::string>> &i : DefaultKeybindings)
        {
            if (keys.exists(i.first))
            {
                for (const std::pair<std::string, std::string> &j : i.second)
                {
                    if (keys[i.first].exists(j.first))
                    {
                        m_Keybindings[i.first][j.first] = std::string(keys[i.first][j.first].c_str());
                    }
                    else
                    {
                        m_Keybindings[i.first][j.first] = DefaultKeybindings.at(i.first).at(j.first);
                    }
                }
            }
            else
            {
                m_Keybindings[i.first] = DefaultKeybindings.at(i.first);
            }
        }
    }
    else
    {
        m_Keybindings = DefaultKeybindings;
    }
}
