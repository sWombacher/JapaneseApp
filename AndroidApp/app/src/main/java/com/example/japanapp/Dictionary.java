package com.example.japanapp;

import android.content.Intent;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.util.Log;
import android.view.View;
import android.widget.EditText;
import android.widget.RadioButton;
import android.widget.TextView;

public class Dictionary extends AppCompatActivity {

    void addButtonHandlers() {
        findViewById(R.id.m_BtnDictionarySearch).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                EditText editText = (EditText)findViewById(R.id.m_DictionarySearchField);
                String searchText = editText.getText().toString();
                String searchResult = "";
                if (((RadioButton)findViewById(R.id.m_DictionaryRadioButtonEnglish)).isChecked())
                    searchResult = m_Translator.translateEnglish(searchText);
                else if (((RadioButton)findViewById(R.id.m_DictionaryRadioButtonKana)).isChecked())
                    searchResult = m_Translator.translateKana(searchText);
                else
                    Log.e("Dictionary", "unknown search language");

                ((TextView)findViewById(R.id.m_DictionaryTextViewResult)).setText(searchResult);
            }
        });
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_dictionary);
        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        getSupportActionBar().setDisplayHomeAsUpEnabled(true);
        this.m_Translator = LogicHandlerAbstraction.getTranslator();
        addButtonHandlers();
    }

    com.cpp.shared.VocabularyTranslator m_Translator;
}
