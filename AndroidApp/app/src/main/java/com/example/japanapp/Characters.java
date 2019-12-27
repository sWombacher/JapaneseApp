package com.example.japanapp;

import android.content.Intent;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;

import android.view.View;


public class Characters extends AppCompatActivity {

    void addButtonHandlers() {
        findViewById(R.id.m_BtnHiragana).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent intent = new Intent(Characters.this, Question.class);
                intent.putExtra(Question.QuestionType, Question.Type.Hiragana.ordinal());
                startActivity(intent);
            }
        });
        findViewById(R.id.m_BtnKatakana).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent intent = new Intent(Characters.this, Question.class);
                intent.putExtra(Question.QuestionType, Question.Type.Katakana.ordinal());
                startActivity(intent);
            }
        });
        findViewById(R.id.m_BtnKana).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent intent = new Intent(Characters.this, Question.class);
                intent.putExtra(Question.QuestionType, Question.Type.Kana.ordinal());
                startActivity(intent);
            }
        });
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_characters);
        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        getSupportActionBar().setDisplayHomeAsUpEnabled(true);

        addButtonHandlers();
    }
}
