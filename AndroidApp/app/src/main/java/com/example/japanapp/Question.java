package com.example.japanapp;

import android.app.Activity;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;

import android.text.Editable;
import android.util.Log;
import android.view.inputmethod.InputMethodManager;
import android.inputmethodservice.Keyboard;
import android.inputmethodservice.KeyboardView;
import android.view.View;
import android.widget.EditText;


public class Question extends AppCompatActivity {

    private int getIntResource(int id) {
        return getResources().getInteger(id);
    }

    private boolean handleSpeacialKeyKeyboardChange(int primaryCode) {
        if (getIntResource(R.integer.keyboard_hiragana0) == primaryCode)
            initializeKeyboard(KeyboardType.Hiragana0);
        else if (getIntResource(R.integer.keyboard_hiragana1) == primaryCode)
            initializeKeyboard(KeyboardType.Hiragana1);
        else if (getIntResource(R.integer.keyboard_katakana0) == primaryCode)
            initializeKeyboard(KeyboardType.Katakana0);
        else if (getIntResource(R.integer.keyboard_katakana1) == primaryCode)
            initializeKeyboard(KeyboardType.Katakana1);
        else if (getIntResource(R.integer.keyboard_romanji) == primaryCode)
            initializeKeyboard(KeyboardType.Romanji);
        else
            return false;
        return true;
    }

    private boolean handleSpecalKeyBackspace(EditText editText, int primaryCode) {
        if (getIntResource(R.integer.keyboard_backspace) != primaryCode)
            return false;

        final int selStart = editText.getSelectionStart();
        final int selEnd = editText.getSelectionEnd();
        Editable editable = editText.getText();
        if (selStart != selEnd) {
            editable.delete(selStart, selEnd);
            editText.setSelection(selStart);
        } else if (selStart > 0){
            editable.delete(selStart - 1, selStart);
            editText.setSelection(selStart - 1);
        }
        return true;
    }

    private boolean handleSpeacialKeyEnter(EditText editText, int primaryCode) {
        if (getIntResource(R.integer.keyboard_enter) != primaryCode)
            return false;

        /// TODO
        return true;
    }

    private boolean handleSpecialKeys(EditText editText, int primaryCode) {
        if (handleSpeacialKeyKeyboardChange(primaryCode))
            return true;

        if (handleSpecalKeyBackspace(editText, primaryCode))
            return true;

        if (handleSpeacialKeyEnter(editText, primaryCode))
            return true;

        return false;
    }

    private KeyboardView.OnKeyboardActionListener mOnKeyboardActionListener = new KeyboardView.OnKeyboardActionListener() {
        @Override public void onKey(int primaryCode, int[] keyCodes) {
            EditText editText = findViewById(R.id.m_AnswerInput);
            if (handleSpecialKeys(editText, primaryCode))
                return;

            Editable editable = editText.getText();
            final int selStart = editText.getSelectionStart();
            final int selEnd = editText.getSelectionEnd();
            editable.replace(selStart, selEnd, "" + (char) primaryCode);
            editText.setSelection(selStart + 1);
        }
        @Override public void onPress(int arg0) { }
        @Override public void onRelease(int primaryCode) { }
        @Override public void onText(CharSequence text) { }
        @Override public void swipeDown() { }
        @Override public void swipeLeft() { }
        @Override public void swipeRight() { }
        @Override public void swipeUp() { }
    };

    public static final String QustionParameter = "QuestionParameter";
    public static final String QuestionType = "QuestionType";
    enum Type {
        Hiragana, Katakana, Kana, IntegerNumbers, FloatingpointNumbers, VocabularyDefault, VocabularyDeck;
    }

    Keyboard mKeyboard;
    KeyboardView mKeyboardView;

    enum KeyboardType {
        Hiragana0, Hiragana1, Katakana0, Katakana1, Romanji
    }
    private static final int AndroidDefaultKeyboardId = -1;
    private int keyboardTypeToId(KeyboardType kbt) {
        switch (kbt) {
            case Hiragana0: return R.xml.hiragana_0;
            case Hiragana1: return R.xml.hiragana_1;
            case Katakana0: return R.xml.katakana_0;
            case Katakana1: return R.xml.hiragana_1;
        }
        return AndroidDefaultKeyboardId;
    }
    private void initializeKeyboard(KeyboardType kbt) {
        final int keyboardTypeId = keyboardTypeToId(kbt);
        if (keyboardTypeId != AndroidDefaultKeyboardId) {
            mKeyboard = new Keyboard(this, keyboardTypeToId(kbt));
            mKeyboardView.setKeyboard(mKeyboard);
        } else {
            mKeyboardView.setPreviewEnabled(false);
            mKeyboard = null;
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_question);
        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        getSupportActionBar().setDisplayHomeAsUpEnabled(true);

        Type type = Type.values()[getIntent().getIntExtra(QuestionType, Type.Kana.ordinal())];
        Log.d("Question", type.toString());

        mKeyboardView = (KeyboardView)findViewById(R.id.keyboardview);
        mKeyboardView.setPreviewEnabled(false);
        mKeyboardView.setOnKeyboardActionListener(mOnKeyboardActionListener);
        initializeKeyboard(KeyboardType.Hiragana0);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        mKeyboard = null;
        mKeyboardView = null;
    }

    public void openKeyboard(View view) {
        mKeyboardView.setVisibility(View.VISIBLE);
        mKeyboardView.setEnabled(true);
        if (view != null)
            ((InputMethodManager)getSystemService(Activity.INPUT_METHOD_SERVICE)).hideSoftInputFromWindow(view.getWindowToken(), 0);
    }
}
