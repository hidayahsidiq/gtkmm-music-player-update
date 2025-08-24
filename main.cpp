#include <gtkmm.h>
#include <SFML/Audio.hpp>
#include <iostream>
#include <iomanip>
#include <sstream>

class MusicPlayer : public Gtk::Window {
public:
    MusicPlayer()
    : play_button("▶ Play"), pause_button("⏸ Pause"),
      stop_button("⏹ Stop"), next_button("⏭ Next"),
      prev_button("⏮ Prev"),
      open_button("📂 Open File")
    {
        set_title("Music Player");
        set_default_size(400, 200);

        // Layout
        vbox.set_margin(10);
        set_child(vbox);

        // Tombol kontrol
        controls.set_spacing(5);
        controls.append(prev_button);
        controls.append(play_button);
        controls.append(pause_button);
        controls.append(stop_button);
        controls.append(next_button);

        vbox.append(controls);
        vbox.append(open_button);

        // Progress bar
        progress.set_hexpand(true);
        vbox.append(progress);

        // Label waktu
        time_label.set_text("00:00 / 00:00");
        vbox.append(time_label);

        // Signal tombol
        open_button.signal_clicked().connect(sigc::mem_fun(*this, &MusicPlayer::on_open_file));
        play_button.signal_clicked().connect(sigc::mem_fun(*this, &MusicPlayer::on_play));
        pause_button.signal_clicked().connect(sigc::mem_fun(*this, &MusicPlayer::on_pause));
        stop_button.signal_clicked().connect(sigc::mem_fun(*this, &MusicPlayer::on_stop));
        next_button.signal_clicked().connect(sigc::mem_fun(*this, &MusicPlayer::on_next));
        prev_button.signal_clicked().connect(sigc::mem_fun(*this, &MusicPlayer::on_prev));

        // Timer update progress
        Glib::signal_timeout().connect(sigc::mem_fun(*this, &MusicPlayer::update_progress), 500);
    }

private:
    Gtk::Box vbox{Gtk::Orientation::VERTICAL};
    Gtk::Box controls{Gtk::Orientation::HORIZONTAL};

    Gtk::Button play_button, pause_button, stop_button, next_button, prev_button, open_button;
    Gtk::Scale progress{Gtk::Orientation::HORIZONTAL};
    Gtk::Label time_label;

    sf::Music music;
    std::vector<std::string> playlist;
    int current_index = -1;
    bool dragging = false;

    // Format detik -> mm:ss
    std::string format_time(sf::Time t) {
        int sec = static_cast<int>(t.asSeconds());
        int m = sec / 60;
        int s = sec % 60;
        std::ostringstream oss;
        oss << std::setw(2) << std::setfill('0') << m << ":"
            << std::setw(2) << std::setfill('0') << s;
        return oss.str();
    }

    void on_open_file() {
        auto dialog = std::make_shared<Gtk::FileChooserDialog>("Pilih File Musik", Gtk::FileChooser::Action::OPEN);
        dialog->add_button("_Batal", Gtk::ResponseType::CANCEL);
        dialog->add_button("_Pilih", Gtk::ResponseType::ACCEPT);

        // Filter audio
        auto filter = Gtk::FileFilter::create();
        filter->set_name("Audio files");
        filter->add_pattern("*.ogg");
        filter->add_pattern("*.wav");
        filter->add_pattern("*.flac");
        filter->add_pattern("*.mp3");
        dialog->add_filter(filter);

        dialog->signal_response().connect([this, dialog](int response) {
            if (response == Gtk::ResponseType::ACCEPT) {
                auto file = dialog->get_file();
                if (file) {
                    std::string path = file->get_path();
                    playlist.push_back(path);
                    if (current_index < 0) {
                        current_index = 0;
                        load_and_play(playlist[current_index]);
                    }
                }
            }
            dialog->hide();
        });

        dialog->set_transient_for(*this);
        dialog->show();
    }

    void load_and_play(const std::string& filename) {
        if (!music.openFromFile(filename)) {
            std::cerr << "Gagal memuat musik: " << filename << std::endl;
            return;
        }
        music.play();
        update_progress();
    }

    void on_play() {
        if (current_index >= 0) music.play();
    }

    void on_pause() {
        if (current_index >= 0) music.pause();
    }

    void on_stop() {
        if (current_index >= 0) music.stop();
    }

    void on_next() {
        if (current_index + 1 < (int)playlist.size()) {
            current_index++;
            load_and_play(playlist[current_index]);
        }
    }

    void on_prev() {
        if (current_index > 0) {
            current_index--;
            load_and_play(playlist[current_index]);
        }
    }

    bool update_progress() {
        if (current_index >= 0) {
            auto dur = music.getDuration();
            auto pos = music.getPlayingOffset();

            double frac = (dur.asSeconds() > 0) ? pos.asSeconds() / dur.asSeconds() : 0;
            progress.set_value(frac * 100);

            time_label.set_text(format_time(pos) + " / " + format_time(dur));
        }
        return true; // keep timer alive
    }
};

int main(int argc, char* argv[]) {
    auto app = Gtk::Application::create("org.example.musicplayer");
    return app->make_window_and_run<MusicPlayer>(argc, argv);
}
