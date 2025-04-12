package com.example.musicplayer;

import android.Manifest; import android.content.ContentResolver; import android.content.pm.PackageManager; import android.database.Cursor; import android.graphics.Bitmap; import android.graphics.BitmapFactory; import android.graphics.drawable.BitmapDrawable; import android.media.MediaMetadataRetriever; import android.media.MediaPlayer; import android.net.Uri; import android.os.Bundle; import android.os.Handler; import android.provider.MediaStore; import android.view.View; import android.widget.AdapterView; import android.widget.BaseAdapter; import android.widget.Button; import android.widget.EditText; import android.widget.ImageView; import android.widget.ListView; import android.widget.SeekBar; import android.widget.TextView; import android.view.LayoutInflater; import android.view.ViewGroup;

import androidx.annotation.NonNull; import androidx.appcompat.app.AppCompatActivity; import androidx.core.app.ActivityCompat; import androidx.core.content.ContextCompat;

import java.io.IOException; import java.util.ArrayList;

public class MainActivity extends AppCompatActivity {

private static final int REQUEST_PERMISSION = 1;
private ListView listView;
private EditText searchInput;
private ArrayList<String> songList;
private ArrayList<String> songPaths;
private ArrayList<Bitmap> albumArts;
private MediaPlayer mediaPlayer;
private ImageView albumArt;
private SeekBar seekBar;
private TextView songDuration;
private Handler handler;
private Runnable updateSeek;
private Button btnPlayPause, btnNext, btnPrev;
private int currentSongIndex = 0;

@Override
protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_main);

    listView = findViewById(R.id.song_list);
    searchInput = findViewById(R.id.search_input);
    albumArt = findViewById(R.id.album_art);
    seekBar = findViewById(R.id.seek_bar);
    songDuration = findViewById(R.id.song_duration);
    btnPlayPause = findViewById(R.id.btn_play_pause);
    btnNext = findViewById(R.id.btn_next);
    btnPrev = findViewById(R.id.btn_prev);

    songList = new ArrayList<>();
    songPaths = new ArrayList<>();
    albumArts = new ArrayList<>();

    handler = new Handler();

    if (ContextCompat.checkSelfPermission(this, Manifest.permission.READ_EXTERNAL_STORAGE)
            != PackageManager.PERMISSION_GRANTED) {
        ActivityCompat.requestPermissions(this,
                new String[]{Manifest.permission.READ_EXTERNAL_STORAGE}, REQUEST_PERMISSION);
    } else {
        loadSongs();
    }

    searchInput.addTextChangedListener(new android.text.TextWatcher() {
        @Override
        public void beforeTextChanged(CharSequence s, int start, int count, int after) {}
        @Override
        public void onTextChanged(CharSequence s, int start, int before, int count) {
            filterSongs(s.toString());
        }
        @Override
        public void afterTextChanged(android.text.Editable s) {}
    });

    listView.setOnItemClickListener(new AdapterView.OnItemClickListener() {
        @Override
        public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
            currentSongIndex = position;
            playSong(songPaths.get(position));
        }
    });

    btnPlayPause.setOnClickListener(v -> togglePlayPause());
    btnNext.setOnClickListener(v -> playNext());
    btnPrev.setOnClickListener(v -> playPrevious());
}

private void loadSongs() {
    ContentResolver contentResolver = getContentResolver();
    Uri uri = MediaStore.Audio.Media.EXTERNAL_CONTENT_URI;
    Cursor cursor = contentResolver.query(uri, null, null, null, null);

    if (cursor != null && cursor.moveToFirst()) {
        int titleColumn = cursor.getColumnIndex(MediaStore.Audio.Media.TITLE);
        int pathColumn = cursor.getColumnIndex(MediaStore.Audio.Media.DATA);

        do {
            String title = cursor.getString(titleColumn);
            String path = cursor.getString(pathColumn);
            songList.add(title);
            songPaths.add(path);
            albumArts.add(getEmbeddedArt(path));
        } while (cursor.moveToNext());

        cursor.close();
    }

    listView.setAdapter(new SongAdapter());
}

private void filterSongs(String query) {
    ArrayList<String> filteredList = new ArrayList<>();
    ArrayList<String> filteredPaths = new ArrayList<>();
    ArrayList<Bitmap> filteredArts = new ArrayList<>();

    for (int i = 0; i < songList.size(); i++) {
        if (songList.get(i).toLowerCase().contains(query.toLowerCase())) {
            filteredList.add(songList.get(i));
            filteredPaths.add(songPaths.get(i));
            filteredArts.add(albumArts.get(i));
        }
    }

    songList = filteredList;
    songPaths = filteredPaths;
    albumArts = filteredArts;
    listView.setAdapter(new SongAdapter());
}

private void playSong(String path) {
    if (mediaPlayer != null && mediaPlayer.isPlaying()) {
        mediaPlayer.stop();
        mediaPlayer.release();
    }
    mediaPlayer = new MediaPlayer();
    try {
        mediaPlayer.setDataSource(path);
        mediaPlayer.prepare();
        mediaPlayer.start();
        updateAlbumArt(path);
        updateSeekBar();
        btnPlayPause.setText("Pause");
    } catch (IOException e) {
        e.printStackTrace();
    }
}

private void togglePlayPause() {
    if (mediaPlayer != null) {
        if (mediaPlayer.isPlaying()) {
            mediaPlayer.pause();
            btnPlayPause.setText("Play");
        } else {
            mediaPlayer.start();
            btnPlayPause.setText("Pause");
        }
    }
}

private void playNext() {
    if (currentSongIndex < songPaths.size() - 1) {
        currentSongIndex++;
        playSong(songPaths.get(currentSongIndex));
    }
}

private void playPrevious() {
    if (currentSongIndex > 0) {
        currentSongIndex--;
        playSong(songPaths.get(currentSongIndex));
    }
}

private void updateAlbumArt(String path) {
    Bitmap bitmap = getEmbeddedArt(path);
    if (bitmap != null) {
        albumArt.setImageBitmap(bitmap);
    } else {
        albumArt.setImageResource(android.R.drawable.ic_menu_gallery);
    }
}

private Bitmap getEmbeddedArt(String path) {
    MediaMetadataRetriever mmr = new MediaMetadataRetriever();
    mmr.setDataSource(path);
    byte[] art = mmr.getEmbeddedPicture();
    if (art != null) {
        return BitmapFactory.decodeByteArray(art, 0, art.length);
    }
    return null;
}

private void updateSeekBar() {
    seekBar.setMax(mediaPlayer.getDuration());

    updateSeek = new Runnable() {
        @Override
        public void run() {
            if (mediaPlayer != null) {
                seekBar.setProgress(mediaPlayer.getCurrentPosition());
                songDuration.setText(millisecondsToTime(mediaPlayer.getCurrentPosition()));
                handler.postDelayed(this, 1000);
            }
        }
    };

    handler.postDelayed(updateSeek, 1000);
}

private String millisecondsToTime(int millis) {
    int minutes = (millis / 1000) / 60;
    int seconds = (millis / 1000) % 60;
    return String.format("%02d:%02d", minutes, seconds);
}

@Override
protected void onDestroy() {
    super.onDestroy();
    if (mediaPlayer != null) {
        mediaPlayer.release();
        mediaPlayer = null;
    }
    if (handler != null && updateSeek != null) {
        handler.removeCallbacks(updateSeek);
    }
}

@Override
public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
    super.onRequestPermissionsResult(requestCode, permissions, grantResults);
    if (requestCode == REQUEST_PERMISSION && grantResults.length > 0
            && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
        loadSongs();
    }
}

class SongAdapter extends BaseAdapter {
    @Override
    public int getCount() {
        return songList.size();
    }

    @Override
    public Object getItem(int position) {
        return songList.get(position);
    }

    @Override
    public long getItemId(int position) {
        return position;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        if (convertView == null) {
            convertView = LayoutInflater.from(MainActivity.this).inflate(android.R.layout.simple_list_item_1, parent, false);
        }

        TextView text = convertView.findViewById(android.R.id.text1);
        text.setText(songList.get(position));
        text.setCompoundDrawablesWithIntrinsicBounds(null, null,
            albumArts.get(position) != null ? new BitmapDrawable(getResources(), albumArts.get(position)) : null,
            null);
        return convertView;
    }
}

}

