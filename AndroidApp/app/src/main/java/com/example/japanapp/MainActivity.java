package com.example.japanapp;

import android.content.Intent;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;

public class MainActivity extends AppCompatActivity {

    void addButtonHandlers() {
        findViewById(R.id.m_BtnCharacters).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent intent = new Intent(MainActivity.this, Characters.class);
                startActivity(intent);
            }
        });
        findViewById(R.id.m_BtnVocabulary).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent intent = new Intent(MainActivity.this, Vocabulary.class);
                startActivity(intent);
            }
        });
        findViewById(R.id.m_BtnDictionary).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent intent = new Intent(MainActivity.this, Dictionary.class);
                startActivity(intent);
            }
        });
        findViewById(R.id.m_BtnNumbers).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent intent = new Intent(MainActivity.this, Numbers.class);
                startActivity(intent);
            }
        });
        findViewById(R.id.m_BtnSettings).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent intent = new Intent(MainActivity.this, GeneralSetttings.class);
                startActivity(intent);
            }
        });
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setTitle("JapanApp - Main");
        setContentView(R.layout.activity_main);
        addButtonHandlers();
        LogicHandlerAbstraction.init(getResources().getAssets(), getFilesDir().toString());
    }
}
