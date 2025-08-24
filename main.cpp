#include <gtkmm.h>
#include <SFML/Audio.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <sstream>

class MusicPlayer : public Gtk::Window {
public:
    MusicPlayer()
    {
        set_title("Music Player");
        set_default_size(500, 400);

        // Layout utama
        set_child(main_box);
        main_box.set_orientation(Gtk::Orientation::VERTICAL);

        // Tombol kontrol
        control_box.set_orientation(Gtk::Orientation::HORIZONTAL);
        main_box.append(control_box);

        btn_open.set_label("📂 Open");
        btn_play.set_label("▶ Play");
        btn_pause.set_label("⏸ Pause");
        btn_stop.set_label("⏹ Stop");
        btn_prev.set_label("⏮ Prev");
        btn_next.set_label("⏭ Next");

        control_box.append(btn_open);
        control_box.append(btn_prev);
        control_box.append(btn_play);
        control_box.append(btn_pause);
        control_box.append(btn_stop);
        control_box.append(btn_next);

        // Progress bar
        progress_box.set_orientation(Gtk::Orientation::HORIZONTAL);
        main_box.append(progress_box);

        progress_box.append(progress_scale);
        progress_box.append(time_label);

        progress_scale.set_draw_value(false);
        progress_scale.set_hexpand(true);
        progress_scale.set_range(0, 100);
        progress_scale.set_value(0);

        time_label.set_text("00:00 / 00:00");

        // Playlist
        main_box.append(playlist_box);
        playlist_box.set_vexpand(true);

        // Hubungkan sinyal tombol
        btn_open.signal_clicked().connect(sigc::mem_fun(*this, &MusicPlayer::on_open));
        btn_play.signal_clicked().connect(sigc::mem_fun(*this, &MusicPlayer::on_play));
        btn_pause.signal_clicked().connect(sigc::mem_fun(*this, &MusicPlayer::on_pause));
        btn_stop.signal_clicked().connect(sigc::mem_fun(*this, &MusicPlayer::on_stop));
        btn_prev.signal_clicked().connect(sigc::mem_fun(*this, &MusicPlayer::on_prev));
        btn_next.signal_clicked().connect(sigc::mem_fun(*this, &MusicPlayer::on_next));

        // Hubungkan klik pada playlist row
        playlist_box.signal_row_activated().connect(sigc::mem_fun(*this, &MusicPlayer::on_row_activated));

        // Timer update progress
        Glib::signal_timeout().connect(sigc::mem_fun(*this, &MusicPlayer::update_progress), 500);
    }

private:
    Gtk::Box main_box, control_box, progress_box;
    Gtk::Button btn_open, btn_play, btn_pause, btn_stop, btn_prev, btn_next;
    Gtk::Scale progress_scale{Gtk::Orientation::HORIZONTAL};
    Gtk::Label time_label;
    Gtk::ListBox playlist_box;

    sf::Music music;
    std::vector<std::string> playlist;
    int current_index = -1;
    bool slider_dragging = false;

    // Open file
    void on_open()
    {
        auto dialog = new Gtk::FileChooserDialog("Select Music File", Gtk::FileChooser::Action::OPEN);
        dialog->set_transient_for(*this);
        dialog->add_button("_Cancel", Gtk::ResponseType::CANCEL);
        dialog->add_button("_Open", Gtk::ResponseType::ACCEPT);

        dialog->signal_response().connect([this, dialog](int response) {
            if (response == Gtk::ResponseType::ACCEPT) {
                auto file = dialog->get_file();
                if (file) {
                    std::string path = file->get_path();
                    playlist.push_back(path);

                    auto row = Gtk::make_managed<Gtk::ListBoxRow>();
                    auto label = Gtk::make_managed<Gtk::Label>(path);
                    row->set_child(*label);
                    playlist_box.append(*row);
                    playlist_box.show();

                    if (current_index < 0) {
                        current_index = 0;
                        load_and_play(playlist[current_index]);
                    }
                }
            }
            dialog->hide();
            delete dialog;
        });

        dialog->show();
    }

    // Play
    void on_play() {
        if (music.getStatus() == sf::SoundSource::Paused)
            music.play();
        else if (current_index >= 0)
            load_and_play(playlist[current_index]);
    }

    // Pause
    void on_pause() { music.pause(); }

    // Stop
    void on_stop() {
        music.stop();
        progress_scale.set_value(0);
    }

    // Previous
    void on_prev() {
        if (current_index > 0) {
            current_index--;
            load_and_play(playlist[current_index]);
        }
    }

    // Next
    void on_next() {
        if (current_index >= 0 && current_index < (int)playlist.size() - 1) {
            current_index++;
            load_and_play(playlist[current_index]);
        }
    }

    // Klik playlist
    void on_row_activated(Gtk::ListBoxRow* row)
    {
        int index = row->get_index();
        if (index >= 0 && index < (int)playlist.size()) {
            current_index = index;
            load_and_play(playlist[current_index]);
        }
    }

    // Load & play
    void load_and_play(const std::string& file)
    {
        if (!music.openFromFile(file)) {
            std::cerr << "Failed to load file: " << file << std::endl;
            return;
        }
        music.play();

        progress_scale.set_range(0, music.getDuration().asSeconds());
        progress_scale.set_value(0);

        // Sinkronkan seek
        progress_scale.signal_change_value().connect(
            sigc::mem_fun(*this, &MusicPlayer::on_slider_changed), false);
    }

    // Update progress
    bool update_progress()
    {
        if (!slider_dragging && music.getStatus() == sf::SoundSource::Playing) {
            progress_scale.set_value(music.getPlayingOffset().asSeconds());
        }

        int cur = music.getPlayingOffset().asSeconds();
        int total = music.getDuration().asSeconds();

        time_label.set_text(format_time(cur) + " / " + format_time(total));
        return true;
    }

    // Seek
    bool on_slider_changed(Gtk::ScrollType, double value)
    {
        slider_dragging = true;
        music.setPlayingOffset(sf::seconds(value));
        slider_dragging = false;
        return true;
    }

    // Format waktu mm:ss
    std::string format_time(int seconds)
    {
        int m = seconds / 60;
        int s = seconds % 60;
        std::ostringstream oss;
        oss << (m < 10 ? "0" : "") << m << ":" << (s < 10 ? "0" : "") << s;
        return oss.str();
    }
};

int main(int argc, char* argv[])
{
    auto app = Gtk::Application::create("org.example.musicplayer");
    return app->make_window_and_run<MusicPlayer>(argc, argv);
}
